/*
 * Copyright 2024 wtcat
 */

#include <tx_api.h>
#include <service/init.h>

#include "stm32h7xx_hal.h"


static void pins_configure(void) {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /*
     * SDMMC
     *
     * PC10  -> D2
     * PC11  -> D3
     * PD2   -> CMD
     * PC12  -> CLK
     * PC8   -> D0
     * PC9   -> D1
     * PD15  -> CD (GPIO)
     */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /*
     * USB Host
     *
     * PA11 - USB_OTG_FS_DM
     * PA12 - USB_OTG_FS_DP
     */
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_FS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
}

static int stm32_pins_init(void) {
    pins_configure();
    return 0;
}

SYSINIT(stm32_pins_init, SI_EARLY_LEVEL, 01);
