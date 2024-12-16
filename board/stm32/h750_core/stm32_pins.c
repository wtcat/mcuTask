/*
 * Copyright 2024 wtcat
 */

#include "tx_api.h"
#include "stm32h7xx_hal.h"


static void pins_configure(void) {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /*
     * PA9  -> USART1_TX
     * PA10 -> USART1_RX
     */
    // GPIO_InitTypeDef GPIO_InitStruct = {0};
    // GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    // GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    // GPIO_InitStruct.Pull = GPIO_NOPULL;
    // GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    // GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    // HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static int stm32_pins_init(void) {
    pins_configure();
    return 0;
}

SYSINIT(stm32_pins_init, SI_EARLY_LEVEL, 01);
