/*
 * Copyright 2024 wtcat
 */
#include <errno.h>
#include <string.h>

#include "basework/generic.h"
#include "tx_api.h"
#include "basework/rte_atomic.h"

#include "stm32h7xx.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_usart.h"


struct stm32_uart {
    const char *name;
    USART_TypeDef *uart;
    int irq;
    rte_atomic32_t refcnt;
};

static const uint32_t datawidth_table[] = {
    LL_USART_DATAWIDTH_7B,
    LL_USART_DATAWIDTH_8B,
    LL_USART_DATAWIDTH_9B
};
static const uint32_t stopbit_table[] = {
    LL_USART_STOPBITS_1,
    LL_USART_STOPBITS_2
};
static struct stm32_uart uart_drivers[] = {
    {
        .name = "uart4",
        .uart = UART4,
        .irq  = UART4_IRQn
    }
};

static void uart_isr(void *arg) {
    
}

static int clk_enable(struct stm32_uart *uart, bool enable) {
    switch (uart->irq) {
    case UART4_IRQn:
        if (enable)
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART4);
        else
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_UART4);
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

int stm32_uart_open(const char *name, void **pdev) {
    if (!name || !pdev)
        return -EINVAL;

    int n = (int)rte_array_size(uart_drivers) - 1;
    int err = -ENODEV;

    while (n >= 0) {
        struct stm32_uart *uart = &uart_drivers[n];
        if (!strcmp(uart->name, name)) {
            if (rte_atomic32_add_return(&uart->refcnt, 1) == 1) {
                err = clk_enable(uart, true);
                if (err)
                    break;

                err = request_irq(uart->irq, uart_isr, uart);
                if (err) {
                    rte_atomic32_sub(&uart->refcnt, 1);
                    break;
                }

                *pdev = uart;
            }
            return 0;
        }
        n--;
    }
    return err;
}

int stm32_uart_close(void *dev) {
    struct stm32_uart *uart = (struct stm32_uart *)dev;

    if (rte_atomic32_read(&uart->refcnt) <= 0)
        return -EINVAL;

    if (rte_atomic32_sub_return(&uart->refcnt, 1) == 0) {
        remove_irq(uart->irq, uart_isr, uart);
        clk_enable(uart, false);
    }

    return 0;
}

int stm32_uart_setup(void *dev, int baudrate, int ndata, int nstop, 
    bool parity, bool odd) {
    struct stm32_uart *uart = (struct stm32_uart *)dev;
    LL_USART_InitTypeDef uartcfg;

    if (uart == NULL)
        return -EINVAL;

    LL_USART_StructInit(&uartcfg);

    if (nstop != 1 && nstop != 2)
        return -EINVAL;

    if (!parity) {
        if (ndata < 7 || ndata > 9)
            return -EINVAL;
        uartcfg.DataWidth = datawidth_table[ndata - 7];
        uartcfg.Parity = LL_USART_PARITY_NONE;
    } else {
        if (ndata < 6 || ndata > 8)
            return -EINVAL;
        uartcfg.DataWidth = datawidth_table[ndata - 6];
        uartcfg.Parity = odd? LL_USART_PARITY_ODD: LL_USART_PARITY_EVEN;
    }
    uartcfg.StopBits = stopbit_table[nstop - 1];
    uartcfg.BaudRate = baudrate;

    return LL_USART_Init(uart->uart, &uartcfg);
}

int stm32_uart_write(void *dev, const char *buf, size_t len) {
    struct stm32_uart *uart = (struct stm32_uart *)dev;
    USART_TypeDef *reg = uart->uart;

    while (len > 0 && !(reg->ISR & LL_USART_ISR_TXE_TXFNF)) {
        reg->TDR = *(uint8_t *)buf;
        buf++;
        len--;
    }
    return 0;
}

int stm32_uart_read(void *dev, char *buf, size_t len) {
    return 0;
}

void __fastcode stm32_uart_putc(char c) {
    USART_TypeDef *reg = UART4;

    while (!(reg->ISR & LL_USART_ISR_TXE_TXFNF));
    reg->TDR = (uint8_t)c;
}

static void stm32_uart_init(void) {

}

rte_sysinit(stm32_uart_init, 200);
