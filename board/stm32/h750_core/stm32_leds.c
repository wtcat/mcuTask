/*
 * Copyright (c) 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE

#include <tx_api.h>
#include <service/init.h>
#include <drivers/gpio.h>
#include <base/bitops.h>
#include <base/log.h>


enum led_node {
    RED,
    GREEN
};

struct led_info {
    struct device *port;
    uint32_t gpio;
    uint16_t ontime;
    uint16_t offtime;
};

static struct delayed_task led_task;
static uint8_t led_node = GREEN;
static bool led_active;
static struct led_info led_array[] = {
    [RED] = {
        .gpio    = STM32_PINMUX('C', 0, GPIO),
        .ontime  = TX_MSEC(1000),
        .offtime = TX_MSEC(1000),
    },
    [GREEN] = {
        .gpio    = STM32_PINMUX('C', 1, GPIO),
        .ontime  = TX_MSEC(100),
        .offtime = TX_MSEC(1500),
    }
};

static inline void stm32_led_on(struct device *gpio, uint32_t pinmux) {
    gpio_pin_set_raw(gpio, STM32_PINMUX_PIN(pinmux), 0);
}

static inline void stm32_led_off(struct device *gpio, uint32_t pinmux) {
    gpio_pin_set_raw(gpio, STM32_PINMUX_PIN(pinmux), 1);
}

static void led_flash_task(struct task *led) {
    const struct led_info *info = &led_array[led_node];
    struct delayed_task *task = to_delayedtask(led);

    if (!led_active) {
        stm32_led_on(info->port, info->gpio);
        delayed_ktask_post(task, info->ontime);
    } else {
        stm32_led_off(info->port, info->gpio);
        delayed_ktask_post(task, info->offtime);
    }

    led_active = !led_active;
}

static int stm32_leds_init(void) {
    for (size_t i = 0; i < rte_array_size(led_array); i++) {
        struct device *gpio;
        char name[] = {"GPIOA"};
        int pin;

        name[4] += STM32_PINMUX_PORT(led_array[i].gpio);
        gpio = device_find(name);
        rte_assert(gpio != NULL);

        int err = gpio_pin_configure(gpio, STM32_PINMUX_PIN(led_array[i].gpio), 
            GPIO_OUTPUT|GPIO_OUTPUT_INIT_HIGH);
        if (err) {
            pr_err("configure gpio(%s: %d) failed\n", name, pin);
            continue;
        }

        led_array[i].port = gpio;
    }

    init_delayed_task(&led_task, led_flash_task);
    delayed_ktask_post(&led_task, TX_MSEC(1000));
 
    return 0;
}

SYSINIT(stm32_leds_init, SI_APPLICATION_LEVEL, 10);
