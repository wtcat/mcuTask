/*
 * Copyright 2024 wtcat
 */
#include <errno.h>
#include <string.h>

#include "tx_api.h"
#include "basework/rte_atomic.h"
#include "basework/container/circ_buffer.h"

#include "stm32h7xx.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_usart.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_dma.h"


struct stm32_uart {
    const char *name;
    USART_TypeDef *reg;
    DMA_TypeDef *dma;
    struct circ_buffer queue;
    int txchan;
    int rxchan;
    int irq;
    uint32_t clksrc;
    uint32_t clkbit;
    rte_atomic32_t refcnt;
};

#define UART_ID(_uart) (int)((_uart) - uart_drivers)

static struct stm32_uart uart_drivers[8] = {
    [4] = {
        .name   = "uart4",
        .reg    = UART4,
        .dma    = DMA1,
        .txchan = LL_DMA_STREAM_0,
        .rxchan = LL_DMA_STREAM_1,
        .irq    = UART4_IRQn,
        .clksrc = LL_RCC_USART234578_CLKSOURCE,
        .clkbit = LL_APB1_GRP1_PERIPH_UART4
    }
};


static void stm32_uart_isr(void *arg) {
    
}

static int stm32_uart_clkset(struct stm32_uart *uart, bool enable) {
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

static struct stm32_uart *stm32_uart_find(const char *name) {
    int ndev = (int)rte_array_size(uart_drivers) - 1;
    while (ndev >= 0) {
        struct stm32_uart *uart = &uart_drivers[ndev];
        if (!strcmp(uart->name, name))
            return uart;
        ndev--;
    }
    return NULL;
}

int uart_open(const char *name, void **pdev) {
    struct stm32_uart *uart;
    int err;

    if (!name || !pdev)
        return -EINVAL;

    uart = stm32_uart_find(name);
    if (!uart)
        return -ENODEV;

    if (rte_atomic32_add_return(&uart->refcnt, 1) == 1) {
        err = stm32_uart_clkset(uart, true);
        if (err)
            goto _failed;

        err = request_irq(uart->irq, stm32_uart_isr, uart);
        if (err)
            goto _clkdis;

        if (uart->dma) {
            LL_DMA_InitTypeDef param;
            if (uart->txchan) {
                LL_DMA_StructInit(&param);
                param.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
                param.PeriphOrM2MSrcAddress = (uint32_t)&uart->reg->TDR;
                param.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
                LL_DMA_Init(uart->dma, uart->txchan, &param);
                LL_USART_ClearFlag_TC(uart->reg);
                LL_USART_EnableIT_TC(uart->reg);
                // LL_USART_EnableDMAReq_TX(USART_TypeDef *USARTx);
            }
            if (uart->rxchan) {
                LL_DMA_StructInit(&param);
                param.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
                param.PeriphOrM2MSrcAddress = (uint32_t)&uart->reg->RDR;
                param.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
                LL_DMA_Init(uart->dma, uart->rxchan, &param);
                LL_USART_ClearFlag_IDLE(uart->reg);
                LL_USART_EnableIT_IDLE(uart->reg);
                // LL_USART_EnableDMAReq_TX(USART_TypeDef *USARTx);
            }

        }
    }

    *pdev = uart;
    return 0;

_clkdis:
    stm32_uart_clkset(uart, false);
_failed:
    rte_atomic32_sub(&uart->refcnt, 1);
    return err;
}

int uart_close(void *dev) {
    struct stm32_uart *uart = (struct stm32_uart *)dev;

    if (rte_atomic32_cmpset(&uart->refcnt.ucnt, 1, 0)) {
        remove_irq(uart->irq, stm32_uart_isr, uart);
        stm32_uart_clkset(uart, false);
    } else {

    }

    return 0;
}

int uart_control(void *dev, unsigned int cmd, void *arg) {
    struct stm32_uart *uart = (struct stm32_uart *)dev;

    if (!uart || !arg)
        return -EINVAL;

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

int uart_write(void *dev, const char *buf, size_t len) {
    struct stm32_uart *uart = (struct stm32_uart *)dev;
    USART_TypeDef *reg = uart->reg;

    while (len > 0 && !(reg->ISR & LL_USART_ISR_TXE_TXFNF)) {
        reg->TDR = *(uint8_t *)buf;
        buf++;
        len--;
    }
    return 0;
}

int uart_read(void *dev, char *buf, size_t len) {
    return 0;
}

void __fastcode console_putc(char c) {
    USART_TypeDef *reg = UART4;

    while (!(reg->ISR & LL_USART_ISR_TXE_TXFNF));
    reg->TDR = (uint8_t)c;
}

static int stm32_uart_init(void) {
    return 0;
}

SYSINIT(stm32_uart_init, 200);
