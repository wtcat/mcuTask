/*
 * Copyright (c) 2024 wtcat
 */


#define TX_USE_BOARD_PRIVATE
#define pr_fmt(fmt) "[gpio-key]: "fmt
#include <tx_api.h>
#include <service/init.h>
#include <base/log.h>
#include <drivers/gpio.h>


struct stm32_gpiokey {
    struct delayed_task task;
    struct gpio_callback cb;
    uint32_t gpio;
    int state;
};

static struct stm32_gpiokey gpio_keys[] = {
    {.gpio = STM32_PINMUX('C', 13, GPIO)}
};

static void stm32_gpiokey_task(struct task *task) {
    struct stm32_gpiokey *key = (struct stm32_gpiokey *)task;
    gpio_port_value_t pval = 0;
    int state;

    gpio_port_get_raw(key->cb.port, &pval);
    state = !!(key->cb.pin_mask & pval);
    if (state == key->state) {
        pr_info("gpiokey state: %d\n", state);
    }
}

static void stm32_gpiokey_isr(struct gpio_callback *cb, gpio_port_pins_t pins) {
    struct stm32_gpiokey *key = rte_container_of(cb, struct stm32_gpiokey, cb);
    gpio_port_value_t pval = 0;

    gpio_port_get_raw(key->cb.port, &pval);
    key->state = !!(pins & pval);
    delayed_ktask_post(&key->task, TX_MSEC(10));
}


static int __rte_unused stm32_gpiokey_init(void) {
    struct device *gpio;
    int err;

    for (size_t i = 0; i < rte_array_size(gpio_keys); i++) {
        char name[] = {"GPIOA"};
        int pin;

        name[4] += STM32_PINMUX_PORT(gpio_keys[i].gpio);
        gpio = device_find(name);
        rte_assert(gpio != NULL);

        pin = STM32_PINMUX_PIN(gpio_keys[i].gpio);
        err = gpio_pin_configure(gpio, pin, GPIO_INPUT | GPIO_PULL_UP);
        if (err) {
            pr_err("configure gpio(%s: %d) failed\n", name, pin);
            continue;
        }

        err = gpio_pin_interrupt_configure(gpio, pin, 
            GPIO_INT_ENABLE | GPIO_INT_EDGE | GPIO_INT_LOW_0 | GPIO_INT_HIGH_1);
        if (err) {
            pr_err("configure gpio(%s: %d) interrupt failed\n", name, pin);
            continue;
        }

        init_delayed_task(&gpio_keys[i].task, stm32_gpiokey_task);
        gpio_keys[i].cb.handler  = stm32_gpiokey_isr;
        gpio_keys[i].cb.pin_mask = BIT(pin);
        gpio_add_callback(gpio, &gpio_keys[i].cb);
    }

    return 0;
}

SYSINIT(stm32_gpiokey_init, SI_APPLICATION_LEVEL, 00);
