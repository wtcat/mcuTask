/*
 * Copyright 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE

#include <errno.h>
#include <stdbool.h>

#include "tx_api.h"
#include "base/bitops.h"
#include "base/container/list.h"


struct stm32_extiline {
	uint16_t irq;
	uint16_t start;
	uint32_t imask;
};

struct stm32_extidesc {
	struct rte_list node;
	void (*fn)(int line, void *arg);
	void *arg;
	uint32_t gpio;
};

static TX_MUTEX mutex;
static struct rte_list exti_heads[16] __fastdata;
static struct stm32_extiline extiline_vec[] __fastdata = {
	{.irq = EXTI0_IRQn, .start = 0, .imask = GENMASK(0, 0)},
	{.irq = EXTI1_IRQn, .start = 1, .imask = GENMASK(1, 1)},
	{.irq = EXTI2_IRQn, .start = 2, .imask = GENMASK(2, 2)},
	{.irq = EXTI3_IRQn, .start = 3, .imask = GENMASK(3, 3)},
	{.irq = EXTI4_IRQn, .start = 4, .imask = GENMASK(4, 4)},
	{.irq = EXTI9_5_IRQn, .start = 5, .imask = GENMASK(9, 5)},
	{
		.irq = EXTI15_10_IRQn,
		.start = 10,
		.imask = GENMASK(15, 10),
	}};

GPIO_TypeDef *stm32_gpio_ports[] __fastdata = {
	GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, 
	GPIOG, GPIOH, GPIOI, GPIOJ, GPIOK,
};

static void __fastcode stm32_exti_isr(void *arg) {
	struct stm32_extiline *line = arg;
	uint32_t pending = EXTI->PR1;
	struct rte_list *pos;

	pending &= line->imask;
	if (pending) {
		/* Clear pending flags */
		EXTI->PR1 = pending;
		rte_list_foreach(pos, &exti_heads[line->start]) {
			struct stm32_extidesc *desc =
				rte_container_of(pos, struct stm32_extidesc, node);
			desc->fn(line->start, desc->arg);
		}
	}
}

static void __fastcode stm32_mulit_exti_isr(void *arg) {
	struct stm32_extiline *line = arg;
	uint32_t pending = EXTI->PR1;
	struct rte_list *pos;

	pending &= line->imask;
	if (pending) {
		unsigned long index;

		/* Clear pending flags */
		EXTI->PR1 = pending;
		do {
			index = ffs(pending) - 1;
			rte_list_foreach(pos, &exti_heads[index]) {
				struct stm32_extidesc *desc =
					rte_container_of(pos, struct stm32_extidesc, node);
				desc->fn(index, desc->arg);
			}
			pending &= ~BIT(index);
		} while (pending);
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
	static const uint32_t lines[] = {
		LL_SYSCFG_EXTI_LINE0,  LL_SYSCFG_EXTI_LINE1,  LL_SYSCFG_EXTI_LINE2,
		LL_SYSCFG_EXTI_LINE3,  LL_SYSCFG_EXTI_LINE4,  LL_SYSCFG_EXTI_LINE5,
		LL_SYSCFG_EXTI_LINE6,  LL_SYSCFG_EXTI_LINE7,  LL_SYSCFG_EXTI_LINE8,
		LL_SYSCFG_EXTI_LINE9,  LL_SYSCFG_EXTI_LINE10, LL_SYSCFG_EXTI_LINE11,
		LL_SYSCFG_EXTI_LINE12, LL_SYSCFG_EXTI_LINE13, LL_SYSCFG_EXTI_LINE14,
		LL_SYSCFG_EXTI_LINE15};
	LL_SYSCFG_SetEXTISource(port, lines[pin]);
}

int gpio_request_irq(uint32_t gpio, void (*fn)(int line, void *arg), 
	void *arg, bool rising_edge, bool falling_edge) {
	struct stm32_extidesc *new;
	struct rte_list *pos;
	uint16_t pin;
	uint16_t port;

	if (fn == NULL)
		return -EINVAL;

	pin = STM32_GPIO_PIN(gpio);
	if (pin > 15)
		return -EINVAL;

	port = STM32_GPIO_PORT(gpio);
	if (port >= rte_array_size(stm32_gpio_ports))
		return -EINVAL;

	guard(os_mutex)(&mutex);

	scoped_guard(os_irq) { 
		rte_list_foreach(pos, &exti_heads[pin]) {
			struct stm32_extidesc *desc = rte_container_of(pos, 
				struct stm32_extidesc, node);
			if (desc->gpio == gpio)
				return -EEXIST;
		}
	}

	new = kmalloc(sizeof(*new), GMF_KERNEL);
	if (new == NULL)
		return -ENOMEM;

	new->fn = fn;
	new->arg = arg;
	new->gpio = gpio;
	scoped_guard(os_irq) { 
		rte_list_add_tail(&new->node, &exti_heads[pin]); 
	}

	/* Configure gpio input */
	LL_GPIO_InitTypeDef iocfg;
	LL_GPIO_StructInit(&iocfg);
	iocfg.Pin = BIT(pin);
	iocfg.Mode = LL_GPIO_MODE_INPUT;
	iocfg.Pull = STM32_GPIO_PULL(gpio);
	LL_GPIO_Init(stm32_gpio_ports[port], &iocfg);

	/* Configure gpio interrupt */
	stm32_set_exti_source(STM32_GPIO_PORT(gpio), pin);
	stm32_exti_trigger(pin, rising_edge, falling_edge);
	stm32_exti_enable(pin, true);

	return 0;
}

int gpio_remove_irq(uint32_t gpio, void (*fn)(int line, void *arg), void *arg) {
	struct stm32_extidesc *target;
	struct rte_list *pos;
	uint16_t pin;

	if (fn == NULL)
		return -EINVAL;

	pin = STM32_GPIO_PIN(gpio);
	if (pin > 15)
		return -EINVAL;

	guard(os_mutex)(&mutex);
	scoped_guard(os_irq) { 
		rte_list_foreach(pos, &exti_heads[pin]) {
			struct stm32_extidesc *desc = rte_container_of(pos, 
				struct stm32_extidesc, node);
			if (desc->gpio == gpio && 
				desc->fn == fn && desc->arg == arg) {
				target = desc;
				goto _found;
			}
		}
	}
	return -ENODEV;

_found:
	scoped_guard(os_irq) { 
		rte_list_del(&target->node); 
	}
	if (rte_list_empty_careful(&exti_heads[pin])) {
		stm32_exti_enable(pin, false);
		stm32_exti_trigger(pin, false, false);
	}

	kfree(target);
	return 0;
}

static int gpio_init_irq(void) {
	tx_mutex_create(&mutex, "gpio_exti", TX_INHERIT);
	for (size_t i = 0; i < rte_array_size(exti_heads); i++)
		RTE_INIT_LIST(&exti_heads[i]);

	for (size_t i = 0; i < rte_array_size(extiline_vec); i++) {
		void (*irqfn)(void *);

		irqfn = (i <= 4) ? stm32_exti_isr : stm32_mulit_exti_isr;
		int err = request_irq(extiline_vec[i].irq, irqfn, &extiline_vec[i]);
		if (err) {
			printk("[err]: %s request irq(%d) failed(%d)\n", __func__,
				   extiline_vec[i].irq, err);
			return err;
		}
	}

	return 0;
}

SYSINIT(gpio_init_irq, SI_PREDRIVER_LEVEL, 10);
