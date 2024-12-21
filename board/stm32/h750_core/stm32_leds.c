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

static struct delayed_task led_task;
static uint8_t led_node = GREEN;
static bool led_active;
static const struct led_info led_array[] = {
    [RED] = {
        .gpio    = STM32_GPIO('C', 0, 0),
        .ontime  = TX_MSEC(1000),
        .offtime = TX_MSEC(1000),
    },
    [GREEN] = {
        .gpio    = STM32_GPIO('C', 1, 0),
        .ontime  = TX_MSEC(100),
        .offtime = TX_MSEC(1500),
    }
};

static inline void stm32_led_on(uint32_t gpio) {
    stm32_pin_set(gpio, 0);
}

static inline void stm32_led_off(uint32_t gpio) {
    stm32_pin_set(gpio, 1);
}

static void led_flash_task(struct task *led) {
    const struct led_info *info = &led_array[led_node];
    struct delayed_task *task = to_delayedtask(led);

    if (!led_active) {
        stm32_led_on(info->gpio);
        delayed_ktask_post(task, info->ontime);
    } else {
        stm32_led_off(info->gpio);
        delayed_ktask_post(task, info->offtime);
    }

    led_active = !led_active;
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

    init_delayed_task(&led_task, led_flash_task);
    delayed_ktask_post(&led_task, TX_MSEC(1000));
 
    return 0;
}

SYSINIT(stm32_leds_init, SI_APPLICATION_LEVEL, 10);
