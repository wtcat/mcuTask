/*
 * Copyright (c) 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE

#include "tx_api.h"


struct stm32_gpiokey {
    struct delayed_task task;
    uint32_t gpio;
    int state;
};

static struct stm32_gpiokey gpio_keys[] = {
    {.gpio = STM32_GPIO('C', 13, LL_GPIO_PULL_UP)}
};


static void stm32_gpiokey_task(struct task *task) {
    struct stm32_gpiokey *key = (struct stm32_gpiokey *)task;
    int state = stm32_pin_get(key->gpio);
    if (state == key->state) {
        // printk("gpiokey state: %d\n", state);
    }
}

static void stm32_gpiokey_isr(int line, void *arg) {
    struct stm32_gpiokey *key = arg;
    (void) line;
    key->state = stm32_pin_get(key->gpio);
    delayed_ktask_post(&key->task, TX_MSEC(10));
}


static int __rte_unused stm32_gpiokey_init(void) {
    for (size_t i = 0; i < rte_array_size(gpio_keys); i++) {
        init_delayed_task(&gpio_keys[i].task, stm32_gpiokey_task);
        gpio_request_irq(gpio_keys[i].gpio, stm32_gpiokey_isr, &gpio_keys[i], true, true);
    }

    return 0;
}

SYSINIT(stm32_gpiokey_init, SI_APPLICATION_LEVEL, 00);
