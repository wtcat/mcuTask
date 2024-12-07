/*
 * Copyright 2024 wtcat
 */
#define TX_USE_BOARD_PRIVATE
#include <errno.h>

#include "tx_api.h"



/*
 * Console port
 */
#define CONSOLE_PORT (USART1)

void __fastcode console_putc(char c) {
    while (!(CONSOLE_PORT->ISR & LL_USART_ISR_TXE));
    CONSOLE_PORT->TDR = (uint8_t)c;
}

int __fastcode console_getc(void) {
    if (CONSOLE_PORT->ISR & LL_USART_ISR_RXNE)
        return CONSOLE_PORT->RDR & 0xFF;
    return -1;
}

static int stm32_uart_init(void) {
    LL_USART_InitTypeDef inits = {
        .BaudRate = 2000000,
        .DataWidth = LL_USART_DATAWIDTH_8B,
        .HardwareFlowControl = LL_USART_HWCONTROL_NONE,
        .OverSampling = LL_USART_OVERSAMPLING_16,
        .Parity = LL_USART_PARITY_NONE,
        .StopBits = LL_USART_STOPBITS_1,
        .TransferDirection = LL_USART_DIRECTION_TX
    };
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
    LL_USART_Init(CONSOLE_PORT, &inits);
    return 0;
}

SYSINIT(stm32_uart_init, SI_MEMORY_LEVEL, 00);
