/*
 * Copyright (c) 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE

#include "tx_api.h"
#include "basework/bitops.h"


enum led_node {
    RED,
    GREEN
};

struct led_info {
    uint32_t gpio;
    uint16_t ontime;
    uint16_t offtime;
};

static TX_TIMER led_timer;
static uint8_t led_node = GREEN;
static bool led_active;
static const struct led_info led_array[] = {
    [RED] = {
        .gpio    = STM32_GPIO('C', 0),
        .ontime  = TX_MSEC(1000),
        .offtime = TX_MSEC(1000),
    },
    [GREEN] = {
        .gpio    = STM32_GPIO('C', 1),
        .ontime  = TX_MSEC(100),
        .offtime = TX_MSEC(1500),
    }
};

static inline void stm32_led_on(uint32_t gpio) {
    int port = STM32_GPIO_PORT(gpio);
    int pin  = STM32_GPIO_PIN(gpio);
    STM32_PIN_CLR(port, BIT(pin));
}

static inline void stm32_led_off(uint32_t gpio) {
    int port = STM32_GPIO_PORT(gpio);
    int pin  = STM32_GPIO_PIN(gpio);
    STM32_PIN_SET(port, BIT(pin));
}

static VOID led_timer_cb(ULONG id) {
    const struct led_info *info = &led_array[led_node];
    (void) id;

    if (!led_active) {
        stm32_led_on(info->gpio);
        tx_timer_change(&led_timer, info->ontime, 0);
    } else {
        stm32_led_off(info->gpio);
        tx_timer_change(&led_timer, info->offtime, 0);
    }
    led_active = !led_active;
    tx_timer_activate(&led_timer);
}

static int stm32_leds_init(void) {
    LL_GPIO_InitTypeDef GPIO_InitStruct;

    LL_GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pin = BIT(STM32_GPIO_PIN(led_array[RED].gpio)) |
                        BIT(STM32_GPIO_PIN(led_array[GREEN].gpio));
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    for (size_t i = 0; i < rte_array_size(led_array); i++)
        stm32_led_off(led_array[i].gpio);

    tx_timer_create(&led_timer, "leds", led_timer_cb, 0,
            TX_MSEC(1000), 0, TX_AUTO_ACTIVATE);    

    return 0;
}

SYSINIT(stm32_leds_init, SI_APPLICATION_LEVEL, 10);
