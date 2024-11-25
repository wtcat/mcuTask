/*
 * Copyright 2024 wtcat
 */
#include <errno.h>
#include <string.h>

#include "tx_api.h"
#include "basework/container/circbuf.h"

#include "stm32h7xx.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_usart.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32_dma.h"
#include "tx_api_extension.h"


struct uart_queue {
    TX_SEMAPHORE sem;
    _CIRBUF_CLASS
    size_t remain;
    const size_t qsize;
};

struct tx_status {
    const char *txbuf;
    size_t len;
    size_t transfered;
    TX_SEMAPHORE idle;
};

struct stm32_uart {
#define to_uart(_dev)(struct stm32_uart *)(_dev)
    const char *name;
    USART_TypeDef *reg;
    union {
        DMA_TypeDef *dma;
        struct stm32_dmastat *dmast;
    };
    TX_MUTEX mtx;
    struct uart_queue rxq;
    struct tx_status txstat;
    bool rs485;
    struct stm32_dmachan txchan;
    struct stm32_dmachan rxchan;
    int irq;
    uint32_t clksrc;
    uint32_t clkbit;
    int refcnt;
    uint32_t rx_drops;

    size_t (*tx_start)(struct stm32_uart *uart, USART_TypeDef *reg, 
        const char *buf, size_t len);
    int (*rx_start)(struct stm32_uart *uart, USART_TypeDef *reg);
};


#define UART_ID(_uart) (int)((_uart)->name[4] - '0')
#define DEFAULT_SPEED 2000000
#define DMA_MAXSEG UINT16_MAX
#define UART_FIFOSIZE 8

#define RS485_SET_TX(_uart) (void)_uart
#define RS485_SET_RX(_uart) (void)_uart


static struct stm32_uart uart_drivers[] = {
    {
        .name    = "uart1",
        .reg     = USART1,
        .dma     = DMA1,
        .txchan  = {-1, LL_DMAMUX1_REQ_USART1_TX}, //LL_DMA_STREAM_0,
        .rxchan  = {LL_DMA_STREAM_0, LL_DMAMUX1_REQ_USART1_RX}, //LL_DMA_STREAM_1,
        .irq     = USART1_IRQn,
        .clksrc  = LL_RCC_USART16_CLKSOURCE,
        .clkbit  = LL_APB2_GRP1_PERIPH_USART1,
        .rxq = {
            .qsize = 256
        }
    },
    {
        .name    = "uart4",
        .reg     = UART4,
        .dma     = DMA1,
        .txchan  = {-1, 0}, //LL_DMA_STREAM_0,
        .rxchan  = {-1, 0}, //LL_DMA_STREAM_1,
        .irq     = UART4_IRQn,
        .clksrc  = LL_RCC_USART234578_CLKSOURCE,
        .clkbit  = LL_APB1_GRP1_PERIPH_UART4,
        .rxq = {
            .qsize = 256
        }
    }
};

static void
txstat_init(struct stm32_uart *uart) {
    tx_semaphore_create(&uart->txstat.idle, (CHAR *)uart->name, 0);
    uart->txstat.transfered = 0;
    uart->txstat.len = 0;
}

static void
txstat_deinit(struct stm32_uart *uart) {
    tx_semaphore_delete(&uart->txstat.idle);
}

static void 
queue_reset(struct uart_queue *q) {
    CIRC_RESET(q);
    q->remain = q->qsize;
}

static int 
queue_create(struct uart_queue *q, const char *name) {
    if (!rte_powerof2(q->qsize))
        return -EINVAL;

    q->buf = kmalloc(q->qsize, GMF_KERNEL);
    if (!q->buf)
        return -ENOMEM;

    queue_reset(q);
    tx_semaphore_create(&q->sem, (CHAR *)name, 0);
    return 0;
}

static void
queue_destroy(struct uart_queue *q) {
    if (q->buf) {
        tx_semaphore_delete(&q->sem);
        kfree(q->buf);
    }
}

static __rte_always_inline DMA_Stream_TypeDef *
stm32_get_dmastream(struct stm32_uart *uart, const struct stm32_dmachan *chan) {
    return (DMA_Stream_TypeDef *)((uint32_t)uart->dma + 
        LL_DMA_STR_OFFSET_TAB[chan->ch]);
}

static void __fastcode
stm32_tx_completed(struct stm32_uart *uart, USART_TypeDef *reg) {
    struct tx_status *txs = &uart->txstat;
    if (txs->transfered < txs->len) {
        size_t bytes = txs->len - txs->transfered;
        txs->transfered += uart->tx_start(uart, uart->reg, 
            txs->txbuf + txs->transfered, bytes);
    } else {
        reg->CR1 &= ~USART_CR1_TCIE;
        reg->CR3 &= ~USART_CR3_DMAT;
        RS485_SET_RX(uart);
        tx_semaphore_ceiling_put(&txs->idle, 1);
    }
}

static size_t __fastcode
stm32_txfifo_start(struct stm32_uart *uart, USART_TypeDef *reg, 
    const char *buf, size_t len) {
    size_t nbytes = 0;
    (void) uart;
    
    len = rte_min(len, UART_FIFOSIZE);
    while (nbytes < len)
        reg->TDR = (uint8_t)buf[nbytes++];

    return nbytes;
}

static int __fastcode __rte_maybe_unused
stm32_rxfifo_recv(struct stm32_uart *uart, USART_TypeDef *reg) {
    uint32_t sr = reg->ISR;
    
    if (sr & LL_USART_ISR_RXNE_RXFNE) {
        struct uart_queue *q = &uart->rxq;
        uint8_t *p = (uint8_t *)q->buf;
        size_t remain = CIRC_SPACE_TO_END(q->head, q->tail, q->qsize);

        while ((reg->ISR & LL_USART_ISR_RXNE_RXFNE)) {
            if (rte_unlikely(remain == 0)) {
               /*
                * If has no more avalible space then drop it
                */
                if (CIRC_SPACE(q->head, q->tail, q->qsize) == 0) {
                    while ((reg->ISR & LL_USART_ISR_RXNE_RXFNE))
                        (void) reg->RDR;
                    uart->rx_drops++;
                    printk("recv: overflow\n");
                    break;
                } else {
                    q->head = 0;
                    remain = CIRC_SPACE_TO_END(q->head, q->tail, q->qsize);
                    continue;
                }
            }
            p[q->head++] = (uint8_t)reg->RDR;
            remain--;
        }
        tx_semaphore_ceiling_put(&q->sem, 1);
    }

    return 0;
}

static size_t __fastcode __rte_maybe_unused
stm32_txdma_start(struct stm32_uart *uart, USART_TypeDef *reg, 
    const char *buf, size_t len) {
    DMA_Stream_TypeDef *stream = stm32_get_dmastream(uart, &uart->txchan);
    uint32_t cr = stream->CR;

    cr &= ~(DMA_SxCR_DIR | DMA_SxCR_CIRC | DMA_SxCR_EN);
    cr |= LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    
    stream->CR   = cr;
    while (stream->CR & DMA_SxCR_EN);
    stm32_dmaicr_write(uart->dmast, &uart->txchan, STM32_DMA_ALLMASK);
    stream->M0AR = (uint32_t)buf;
    stream->NDTR = len;
    stream->CR  |= DMA_SxCR_EN;
    return len;
}

static int __rte_maybe_unused
stm32_rxdma_recv(struct stm32_uart *uart, USART_TypeDef *reg) {
    DMA_Stream_TypeDef *stream = stm32_get_dmastream(uart, &uart->rxchan);
    struct uart_queue *q = &uart->rxq;
    uint32_t remain = stream->NDTR;
    uint32_t space = CIRC_SPACE(q->head, q->tail, q->qsize);
    uint32_t bytes;

    if (q->remain > remain)
        bytes = q->remain - remain;
    else
        bytes = q->remain + q->qsize - remain;

    q->remain = remain;
    if (bytes <= space) {
        q->head = (q->head + bytes) & (q->qsize - 1);
        tx_semaphore_ceiling_put(&q->sem, 1);
    } else {
        stream->CR &= ~DMA_SxCR_EN;
        while (stream->CR & DMA_SxCR_EN);
        queue_reset(q);
        stream->CR |= DMA_SxCR_EN;
        uart->rx_drops++;
    }

    return 0;
}

static int __rte_maybe_unused
stm32_rxdma_prepare(struct stm32_uart *uart) {
    DMA_Stream_TypeDef *stream = stm32_get_dmastream(uart, &uart->rxchan);
    uint32_t cr = stream->CR;

    LL_USART_EnableDMAReq_RX(uart->reg);
    cr &= ~(DMA_SxCR_DIR | DMA_SxCR_CIRC | DMA_SxCR_EN);
    cr |= LL_DMA_DIRECTION_PERIPH_TO_MEMORY | DMA_SxCR_CIRC;
    stream->CR   = cr;
    while (stream->CR & DMA_SxCR_EN);
    stm32_dmaicr_write(uart->dmast, &uart->rxchan, STM32_DMA_ALLMASK);
    queue_reset(&uart->rxq);
    stream->M0AR = (uint32_t)uart->rxq.buf;
    stream->NDTR = uart->rxq.qsize;
    stream->CR  |= DMA_SxCR_EN;
    return 0;
}

static void __rte_maybe_unused
stm32_dma_stop(struct stm32_uart *uart, const struct stm32_dmachan *chan) {
    DMA_Stream_TypeDef *stream = stm32_get_dmastream(uart, chan);
    stream->CR &= ~DMA_SxCR_EN;
    while (stream->CR & DMA_SxCR_EN);
}

static void __fastcode
stm32_uart_isr(void *arg) {
    struct stm32_uart *uart = (struct stm32_uart *)arg;
    USART_TypeDef *reg = uart->reg;
    uint32_t sr = reg->ISR;

    /* Clear interrupt pending bits */
    reg->ICR = sr;

    /* Idle interrupt */
    if (sr & (LL_USART_ISR_IDLE | LL_USART_ISR_RXFT))
        uart->rx_start(uart, reg);

    /* Tansmit completed */
    if (sr & LL_USART_ISR_TC)
        stm32_tx_completed(uart, reg);
}

static int 
stm32_uart_clkset(struct stm32_uart *uart, bool enable) {
    uint32_t devid = UART_ID(uart);

    if (devid == 1 || devid == 6) {
        if (enable)
            LL_APB2_GRP1_EnableClock(uart->clkbit);
        else
            LL_APB2_GRP1_DisableClock(uart->clkbit);
    } else {
        if (enable)
            LL_APB1_GRP1_EnableClock(uart->clkbit);
        else
            LL_APB1_GRP1_DisableClock(uart->clkbit);
    }
    return 0;
}

static struct stm32_uart *
stm32_uart_find(const char *name) {
    int ndev = (int)rte_array_size(uart_drivers) - 1;
    while (ndev >= 0) {
        struct stm32_uart *uart = &uart_drivers[ndev];
        if (!strcmp(uart->name, name))
            return uart;
        ndev--;
    }
    return NULL;
}

static int 
stm32_uart_control(struct stm32_uart *uart, unsigned int cmd, void *arg) {
    switch (cmd) {
    case UART_SET_FORMAT: {
        static const uint32_t datawidth_table[] = {
            LL_USART_DATAWIDTH_7B,
            LL_USART_DATAWIDTH_8B,
            LL_USART_DATAWIDTH_9B
        };
        static const uint32_t stopbit_table[] = {
            LL_USART_STOPBITS_1,
            LL_USART_STOPBITS_2
        };
        struct uart_param *p = (struct uart_param *)arg;
        uint32_t cr1 = uart->reg->CR1;
        uint32_t clkfreq;

        cr1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE | 
            USART_CR1_RE | USART_CR1_OVER8 | USART_CR1_UE);
        if (p->parity != kUartParityNone) {
            if (p->parity == kUartParityOdd)
                cr1 |= LL_USART_PARITY_ODD;
            else
                cr1 |= LL_USART_PARITY_EVEN;
            cr1 |= datawidth_table[p->nb_data + 1];
        } else {
            cr1 |= LL_USART_PARITY_NONE;
            cr1 |= datawidth_table[p->nb_data];
        }

        clkfreq = LL_RCC_GetUSARTClockFreq(uart->clksrc);
        scoped_guard(os_irq) {
            uart->reg->CR1 = cr1;
            LL_USART_SetPrescaler(uart->reg, 0);
            LL_USART_SetBaudRate(uart->reg, clkfreq, 0, 0, p->baudrate);
            LL_USART_SetStopBitsLength(uart->reg, stopbit_table[p->nb_stop]);
            if (p->hwctrl)
                LL_USART_SetHWFlowCtrl(uart->reg, USART_CR3_RTSE | USART_CR3_CTSE);
            else
                LL_USART_SetHWFlowCtrl(uart->reg, 0);
            LL_USART_EnableDirectionTx(uart->reg);
            LL_USART_EnableDirectionRx(uart->reg);
            uart->reg->CR1 |= USART_CR1_UE;
        }
        break;
    }
    case UART_SET_SPEED: {
        uint32_t clkfreq = LL_RCC_GetUSARTClockFreq(uart->clksrc);
        uint32_t baudrate = *(uint32_t *)arg;
        scoped_guard(os_irq) {
            uart->reg->CR1 &= ~USART_CR1_UE;
            LL_USART_SetBaudRate(uart->reg, clkfreq, 0, 0, baudrate);
            uart->reg->CR1 |= USART_CR1_UE;
        }
        break;
    }
    default:
        break;
    }

    return 0;
}

int uart_open(const char *name, void **pdev) {
    struct stm32_uart *uart;
    struct uart_param param;
    int err;

    if (!name || !pdev)
        return -EINVAL;

    uart = stm32_uart_find(name);
    if (!uart)
        return -ENODEV;

    guard(os_mutex)(&uart->mtx);
    if (uart->refcnt++ == 0) {
        err = queue_create(&uart->rxq, uart->name);
        if (err)
            goto _failed;

        err = request_irq(uart->irq, stm32_uart_isr, uart);
        if (err)
            goto _del_rxq;

        err = stm32_uart_clkset(uart, true);
        if (err)
            goto _remove_irq;

        if (uart->dma) {
            LL_DMA_InitTypeDef param;
            if (stm32_dmachan_valid(&uart->txchan)) {
                LL_DMA_StructInit(&param);
                param.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
                param.PeriphOrM2MSrcAddress = (uint32_t)&uart->reg->TDR;
                param.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
                param.PeriphRequest = uart->txchan.id;
                LL_DMA_Init(uart->dma, uart->txchan.ch, &param);
            }
            if (stm32_dmachan_valid(&uart->rxchan)) {
                uart->rx_start = stm32_rxdma_recv;
                LL_DMA_StructInit(&param);
                param.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
                param.PeriphOrM2MSrcAddress = (uint32_t)&uart->reg->RDR;
                param.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
                param.PeriphRequest = uart->rxchan.id;
                LL_DMA_Init(uart->dma, uart->rxchan.ch, &param);
            }
        }

        /* Enable FIFO */
        LL_USART_SetTXFIFOThreshold(uart->reg, 0);
        LL_USART_SetRXFIFOThreshold(uart->reg, 2);
        LL_USART_EnableFIFO(uart->reg);
        
       if (!uart->tx_start) 
            uart->tx_start = stm32_txfifo_start;

        if (!uart->rx_start) {
            uart->rx_start = stm32_rxfifo_recv;
            LL_USART_DisableDMAReq_RX(uart->reg);
            LL_USART_EnableIT_RXFT(uart->reg);
        }

        LL_USART_ClearFlag_IDLE(uart->reg);
        LL_USART_EnableIT_IDLE(uart->reg);

        txstat_init(uart);

        param.baudrate = DEFAULT_SPEED;
        param.hwctrl   = false;
        param.nb_data  = kUartDataWidth_8B;
        param.nb_stop  = kUartStopWidth_1B;
        param.parity   = kUartParityNone;
        (void) stm32_uart_control(uart, UART_SET_FORMAT, &param);
        if (stm32_dmachan_valid(&uart->rxchan))
            (void) stm32_rxdma_prepare(uart);
    }

    *pdev = uart;
    return 0;

_remove_irq:
    remove_irq(uart->irq, stm32_uart_isr, uart);
_del_rxq:
    queue_destroy(&uart->rxq);
_failed:
    uart->refcnt--;
    return err;
}

int uart_close(void *dev) {
    struct stm32_uart *uart = (struct stm32_uart *)dev;

    if (!stm32_uart_find(uart->name))
        return -ENODEV;

    guard(os_mutex)(&uart->mtx);
    if (uart->refcnt > 0) {
        if (--uart->refcnt == 0) {
            if (stm32_dmachan_valid(&uart->txchan))
                stm32_dma_stop(uart, &uart->txchan);
            if (stm32_dmachan_valid(&uart->rxchan))
                stm32_dma_stop(uart, &uart->rxchan);
            stm32_uart_clkset(uart, false);
            remove_irq(uart->irq, stm32_uart_isr, uart);
            queue_destroy(&uart->rxq);
            txstat_deinit(uart);
        }
    }

    return 0;
}

int uart_control(void *dev, unsigned int cmd, void *arg) {
    struct stm32_uart *uart = (struct stm32_uart *)dev;

    if (!uart || !arg)
        return -EINVAL;
    
    guard(os_mutex)(&uart->mtx);
    return stm32_uart_control(dev, cmd, arg);
}

ssize_t __fastcode 
uart_write(void *dev, const char *buf, size_t len, unsigned int options) {
    struct stm32_uart *uart = (struct stm32_uart *)dev;
    struct tx_status *txs = &uart->txstat;
    USART_TypeDef *reg = uart->reg;

    guard(os_mutex)(&uart->mtx);
    
    txs->txbuf = buf;
    txs->len = len;

    if (stm32_dmachan_valid(&uart->txchan) && len > UART_FIFOSIZE) {
        SCB_CleanDCache_by_Addr((uint32_t *)buf, len);
        uart->tx_start = stm32_txdma_start;
        reg->CR3 |= USART_CR3_DMAT;
        scoped_guard(os_irq) {
            RS485_SET_TX(uart);
            txs->transfered = stm32_txdma_start(uart, reg, txs->txbuf, len);
            reg->CR1 |= USART_CR1_TCIE;
        }
    } else {
        reg->CR3 &= ~USART_CR3_DMAT;
        uart->tx_start = stm32_txfifo_start;
        scoped_guard(os_irq) {
            RS485_SET_TX(uart);
            reg->CR1 |= USART_CR1_TCIE;
            txs->transfered = stm32_txfifo_start(uart, reg, txs->txbuf, len);
        }
    }

    tx_semaphore_get(&uart->txstat.idle, TX_WAIT_FOREVER);
    return 0;
}

ssize_t __fastcode
uart_read(void *dev, char *buf, size_t len, unsigned int options) {
    struct stm32_uart *uart = to_uart(dev);
    struct uart_queue *q;
    size_t ret;

    if (!uart || !buf)
        return -EINVAL;

    guard(os_mutex)(&uart->mtx);
    q = &uart->rxq;
    ret = 0;

    while (len > 0) {
        size_t used = CIRC_CNT(q->head, q->tail, q->qsize);
        size_t remain = CIRC_CNT_TO_END(q->head, q->tail, q->qsize);

        if (used > 0) {
            if (remain == 0) {
                q->tail = 0;
                remain = CIRC_CNT_TO_END(q->head, q->tail, q->qsize);
            }
            
            size_t bytes = rte_min_t(size_t, remain, len);
            if (stm32_dmachan_valid(&uart->rxchan))
                SCB_InvalidateDCache_by_Addr(q->buf + q->tail, bytes);

            memcpy(buf + ret, q->buf + q->tail, bytes);
            q->tail += bytes;
            len     -= bytes;
            ret     += bytes;
        } else if (!ret && !(options & O_NOBLOCK)) {
            tx_semaphore_get(&q->sem, TX_WAIT_FOREVER);
        } else {
            break;
        }
    }

    return ret;
}

void __fastcode console_putc(char c) {
    USART_TypeDef *reg = UART4;

    while (!(reg->ISR & LL_USART_ISR_TXE_TXFNF));
    reg->TDR = (uint8_t)c;
}

static int stm32_uart_init(void) {
    for (size_t i = 0; i < rte_array_size(uart_drivers); i++) {
        struct stm32_uart *uart = uart_drivers + i;
        tx_mutex_create(&uart->mtx, (char *)uart->name, TX_INHERIT);
    }
    return 0;
}

SYSINIT(stm32_uart_init, SI_DRIVER_ORDER);
