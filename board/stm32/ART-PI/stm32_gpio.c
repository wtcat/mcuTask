/*
 * Copyright 2024 wtcat
 */

#include "basework/generic.h"
#define TX_USE_BOARD_PRIVATE

#include <errno.h>
#include <stdbool.h>

#include "tx_api.h"
#include "basework/container/list.h"
#include "basework/bitops.h"

#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_system.h"

#define _ERROR "Error<gpio_intr>: "

#define STM32_PINPORT_SHIFT 8
#define STM32_GPIO(_p, _n)   ((((_p) - 'A') << STM32_PINPORT_SHIFT) | (_n))
#define STM32_PIN(_gpio)     ((_gpio) & 0xFF)
#define STM32_PINPORT(_gpio) (((_gpio) >> STM32_PINPORT_SHIFT) & 0xFF)

#ifndef BIT
#define BIT(n) (0x1ul << (n))
#endif

struct stm32_extiline {
    uint16_t irq;
    uint16_t start;
    uint32_t imask;
    void (*cb)(void *);
};

struct stm32_extidesc {
    struct rte_list node;
    void (*fn)(int line, void *arg);
    void *arg;
    uint32_t gpio;
};

static void stm32_exti_isr(void *arg);
static void stm32_mulit_exti_isr(void *arg);

static struct rte_list exti_heads[16] __fastdata;
static struct stm32_extiline extiline_vec[] __fastdata = {
    {
        .irq = EXTI0_IRQn,
        .start = 0,
        .imask = GENMASK(0, 0),
        .cb = stm32_exti_isr
    },
    {
        .irq = EXTI1_IRQn,
        .start = 1,
        .imask = GENMASK(1, 1),
        .cb = stm32_exti_isr
    },
    {
        .irq = EXTI2_IRQn,
        .start = 2,
        .imask = GENMASK(2, 2),
        .cb = stm32_exti_isr
    },
    {
        .irq = EXTI3_IRQn,
        .start = 3,
        .imask = GENMASK(3, 3),
        .cb = stm32_exti_isr
    },
    {
        .irq = EXTI9_5_IRQn,
        .start = 5,
        .imask = GENMASK(9, 5),
        .cb = stm32_mulit_exti_isr
    },
    {
        .irq = EXTI15_10_IRQn,
        .start = 10,
        .imask = GENMASK(15, 10),
        .cb = stm32_mulit_exti_isr
    }
};

static void __fastcode stm32_exti_isr(void *arg) {
    struct stm32_extiline *line = arg;
    uint32_t pending = EXTI->PR1;
    struct rte_list *pos;

    pending &= line->imask;

    if (pending) {
        /* Clear pending flags */
        EXTI->PR1 = pending;

        rte_list_foreach(pos, &exti_heads[]) {
            struct stm32_extidesc *desc = rte_container_of(pos, 
                struct stm32_extidesc, node);
            desc->fn(line->start, desc->arg);
        }
    }
}

static void __fastcode stm32_mulit_exti_isr(void *arg) {
    struct stm32_extiline *line = arg;
    uint32_t pending = EXTI->PR1;
    struct rte_list *pos;
    unsigned long index;

    pending &= line->imask;

    if (pending) {
        /* Clear pending flags */
        EXTI->PR1 = pending;

        do {
            index = ffs(pending) - 1;
            rte_list_foreach(pos, &exti_heads[index]) {
                struct stm32_extidesc *desc = rte_container_of(pos, 
                    struct stm32_extidesc, node);
                desc->fn(index, desc->arg);
            }
            index &= ~BIT(index);
        } while (index);
    }
}


static int stm32_exti_trigger(int line, bool rising_edge, bool falling_edge) {
    if (rising_edge)
        LL_EXTI_EnableRisingTrig_0_31(BIT(line));
    else
        LL_EXTI_DisableRisingTrig_0_31(BIT(line));

    if (falling_edge)
        LL_EXTI_EnableFallingTrig_0_31(BIT(line));
    else
        LL_EXTI_DisableFallingTrig_0_31(BIT(line));

    return 0;
}

static int stm32_exti_enable(int line, bool enable) {
    if (line > 31)
        return -EINVAL;

    if (enable)
        LL_EXTI_EnableIT_0_31(BIT(line));
    else
        LL_EXTI_DisableIT_0_31(BIT(line));
    return 0;
}

static void stm32_set_exti_source(int port, int pin) {
    LL_SYSCFG_SetEXTISource(port, pin);
}

static void stm32_exti_shared_isr(void *arg) {
    struct stm32_extiline *line = arg;
    uint32_t pending = EXTI->PR1;
    struct rte_list *pos;

    /* Clear pending flags */
    EXTI->PR1 = pending;

    rte_list_foreach(pos, &line->head) {

        struct stm32_extidesc *desc = rte_container_of(pos, 
            struct stm32_extidesc, node);
        desc->fn(line->start, desc->arg);
    }
}

static struct stm32_extiline *find_extiline(uint16_t pin) {
    for (size_t i = 0; i < rte_array_size(exti_vectbl); i++) {
        if (pin <= exti_vectbl[i].end &&
            pin >= exti_vectbl[i].end)
            return &exti_vectbl[i];
    }
    return NULL;
}

int gpio_init_irq(void) {
    for (size_t i = 0; i < rte_array_size(extiline_vec); i++) {
        int err = request_irq(extiline_vec[i].irq, extiline_vec[i].cb, 
            &extiline_vec[i]);
        if (err) {
            printk(_ERROR"%s request irq(%d) failed(%d)\n", __func__, 
                extiline_vec[i].irq, err);
            return err;
        }
    }
    for (size_t i = 0; i < rte_array_size(exti_heads); i++)
        RTE_INIT_LIST(&exti_heads[i]);
    
    return 0;
}

int gpio_request_irq(uint32_t gpio, void (*fn)(int line, void *arg), void *arg, 
    bool rising_edge, bool falling_edge) {
    struct stm32_extiline *line;
    struct stm32_extidesc *new;
    struct rte_list *pos;
    uint16_t pin;

    if (fn == NULL)
        return -EINVAL;
    
    pin = STM32_PIN(gpio);
    if (pin > 31)
        return -EINVAL;

    line = find_extiline(pin);
    if (line == NULL)
        return -ENODEV;

    rte_list_foreach(pos, &line->head) {
        struct stm32_extidesc *desc = rte_container_of(pos, 
            struct stm32_extidesc, node);
        if (desc->gpio == gpio)
            return -EEXIST;
    }

    new = kmalloc(sizeof(*new), GMF_KERNEL);
    if (new == NULL)
        return -ENOMEM;

    new->fn   = fn;
    new->arg  = arg;
    new->gpio = gpio;
    rte_list_add_tail(&new->node, &line->head);

    stm32_set_exti_source(STM32_PINPORT(gpio), pin);
    stm32_exti_trigger(pin, rising_edge, falling_edge);
    stm32_exti_enable(pin, true);

    return 0;
}
