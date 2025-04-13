/*
 * Copyright 2024 wtcat
 */

#define pr_fmt(fmt) "[gpio]: "fmt
#define TX_USE_BOARD_PRIVATE

#include <errno.h>
#include <stdbool.h>

#include <tx_api.h>
#include <base/log.h>
#include <base/bitops.h>

#define _GPIO_SOURCE_FILE
#include <drivers/gpio.h>


/**
 * @brief PIN configuration bitfield
 *
 * Pin configuration is coded with the following
 * fields
 *    Alternate Functions [ 0 : 3 ]
 *    GPIO Mode           [ 4 : 5 ]
 *    GPIO Output type    [ 6 ]
 *    GPIO Speed          [ 7 : 8 ]
 *    GPIO PUPD config    [ 9 : 10 ]
 *    GPIO Output data     [ 11 ]
 *
 */

/* GPIO Mode */
#define STM32_MODER_INPUT_MODE		(0x0 << STM32_MODER_SHIFT)
#define STM32_MODER_OUTPUT_MODE		(0x1 << STM32_MODER_SHIFT)
#define STM32_MODER_ALT_MODE		(0x2 << STM32_MODER_SHIFT)
#define STM32_MODER_ANALOG_MODE		(0x3 << STM32_MODER_SHIFT)
#define STM32_MODER_MASK	 	0x3
#define STM32_MODER_SHIFT		4

/* GPIO Output type */
#define STM32_OTYPER_PUSH_PULL		(0x0 << STM32_OTYPER_SHIFT)
#define STM32_OTYPER_OPEN_DRAIN		(0x1 << STM32_OTYPER_SHIFT)
#define STM32_OTYPER_MASK		0x1
#define STM32_OTYPER_SHIFT		6

/* GPIO High impedance/Pull-up/pull-down */
#define STM32_PUPDR_NO_PULL		(0x0 << STM32_PUPDR_SHIFT)
#define STM32_PUPDR_PULL_UP		(0x1 << STM32_PUPDR_SHIFT)
#define STM32_PUPDR_PULL_DOWN		(0x2 << STM32_PUPDR_SHIFT)
#define STM32_PUPDR_MASK		0x3
#define STM32_PUPDR_SHIFT		9

/* GPIO plain output value */
#define STM32_ODR_0			(0x0 << STM32_ODR_SHIFT)
#define STM32_ODR_1			(0x1 << STM32_ODR_SHIFT)
#define STM32_ODR_MASK			0x1
#define STM32_ODR_SHIFT			11

/**
 * Configures a GPIO pin to power on the system after Poweroff.
 * This flag is reserved to GPIO pins that are associated with wake-up pins
 * in STM32 PWR devicetree node, through the property "wkup-gpios".
 */
#define STM32_GPIO_WKUP (1 << 8)

/** @cond INTERNAL_HIDDEN */
#define STM32_GPIO_SPEED_SHIFT 9
#define STM32_GPIO_SPEED_MASK  0x3
/** @endcond */

/** Configure the GPIO pin output speed to be low */
#define STM32_GPIO_LOW_SPEED (0x0 << STM32_GPIO_SPEED_SHIFT)

/** Configure the GPIO pin output speed to be medium */
#define STM32_GPIO_MEDIUM_SPEED (0x1 << STM32_GPIO_SPEED_SHIFT)

/** Configure the GPIO pin output speed to be high */
#define STM32_GPIO_HIGH_SPEED (0x2 << STM32_GPIO_SPEED_SHIFT)

/** Configure the GPIO pin output speed to be very high */
#define STM32_GPIO_VERY_HIGH_SPEED (0x3 << STM32_GPIO_SPEED_SHIFT)


#define STM32_PINCFG_MODE_OUTPUT        STM32_MODER_OUTPUT_MODE
#define STM32_PINCFG_MODE_INPUT         STM32_MODER_INPUT_MODE
#define STM32_PINCFG_MODE_ANALOG        STM32_MODER_ANALOG_MODE
#define STM32_PINCFG_PUSH_PULL          STM32_OTYPER_PUSH_PULL
#define STM32_PINCFG_OPEN_DRAIN         STM32_OTYPER_OPEN_DRAIN
#define STM32_PINCFG_PULL_UP            STM32_PUPDR_PULL_UP
#define STM32_PINCFG_PULL_DOWN          STM32_PUPDR_PULL_DOWN
#define STM32_PINCFG_FLOATING           STM32_PUPDR_NO_PULL


struct stm32_gpio {
#define to_stm32gpio(port) (struct stm32_gpio *)(port)
	struct gpio_device dev;
	GPIO_TypeDef *gpio;
};

#define _GPIO_ITEM(port) { .dev = { .name = #port }, .gpio = port }

struct stm32_extiline {
	uint16_t irq;
	uint16_t start;
	uint32_t imask;
};

static STAILQ_HEAD(, gpio_callback) exti_heads[16] __fastdata;
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
	}
};
static struct stm32_gpio stm32_gpio_port[] = {
	_GPIO_ITEM(GPIOA),
	_GPIO_ITEM(GPIOB),
	_GPIO_ITEM(GPIOC),
	_GPIO_ITEM(GPIOD),
	_GPIO_ITEM(GPIOE),
	_GPIO_ITEM(GPIOF),
	_GPIO_ITEM(GPIOG),
	_GPIO_ITEM(GPIOH),
	_GPIO_ITEM(GPIOI),
	_GPIO_ITEM(GPIOJ),
	_GPIO_ITEM(GPIOK)
};

static void stm32_exti_isr(void *arg) {
	struct stm32_extiline *line = arg;
	uint32_t pending = EXTI->PR1;
	struct gpio_callback *pos;

	pending &= line->imask;
	if (pending) {
		/* Clear pending flags */
		EXTI->PR1 = pending;
		STAILQ_FOREACH(pos, &exti_heads[line->start], node) {
			pos->handler(pos, BIT(line->start));
		}
	}
}

static void stm32_mulit_exti_isr(void *arg) {
	struct stm32_extiline *line = arg;
	uint32_t pending = EXTI->PR1;
	struct gpio_callback *pos;

	pending &= line->imask;
	if (pending) {
		unsigned long index;

		/* Clear pending flags */
		EXTI->PR1 = pending;
		do {
			index = ffs(pending) - 1;
			STAILQ_FOREACH(pos, &exti_heads[index], node) {
				pos->handler( pos, BIT(index));
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
		LL_SYSCFG_EXTI_LINE15
	};
	LL_SYSCFG_SetEXTISource(port, lines[pin]);
}

static int stm32_gpio_flags_to_conf(gpio_flags_t flags, uint32_t *pincfg) {
	if ((flags & GPIO_OUTPUT) != 0) {
		/* Output only or Output/Input */
		*pincfg = STM32_PINCFG_MODE_OUTPUT;
		if ((flags & GPIO_SINGLE_ENDED) != 0) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				*pincfg |= STM32_PINCFG_OPEN_DRAIN;
			} else  {
				/* Output can't be open source */
				return -ENOTSUP;
			}
		} else {
			*pincfg |= STM32_PINCFG_PUSH_PULL;
		}

		if ((flags & GPIO_PULL_UP) != 0) {
			*pincfg |= STM32_PINCFG_PULL_UP;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			*pincfg |= STM32_PINCFG_PULL_DOWN;
		}
	} else if  ((flags & GPIO_INPUT) != 0) {
		/* Input */
		*pincfg = STM32_PINCFG_MODE_INPUT;
		if ((flags & GPIO_PULL_UP) != 0) {
			*pincfg |= STM32_PINCFG_PULL_UP;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			*pincfg |= STM32_PINCFG_PULL_DOWN;
		} else {
			*pincfg |= STM32_PINCFG_FLOATING;
		}
	} else {
		/* Deactivated: Analog */
		*pincfg = STM32_PINCFG_MODE_ANALOG;
	}

	switch (flags & (STM32_GPIO_SPEED_MASK << STM32_GPIO_SPEED_SHIFT)) {
	case STM32_GPIO_VERY_HIGH_SPEED:
		*pincfg |= STM32_OSPEEDR_VERY_HIGH_SPEED;
		break;
	case STM32_GPIO_HIGH_SPEED:
		*pincfg |= STM32_OSPEEDR_HIGH_SPEED;
		break;
	case STM32_GPIO_MEDIUM_SPEED:
		*pincfg |= STM32_OSPEEDR_MEDIUM_SPEED;
		break;
	default:
		*pincfg |= STM32_OSPEEDR_LOW_SPEED;
		break;
	}
	return 0;
}

static void gpio_stm32_configure_raw(struct stm32_gpio *dev, gpio_pin_t pin,
	uint32_t conf, uint32_t func) {
	GPIO_TypeDef *gpio = dev->gpio;
	uint32_t pin_ll = BIT(pin);
	uint32_t mode;
	uint32_t otype;
	uint32_t ospeed;
	uint32_t pupd;

	mode = conf & (STM32_MODER_MASK << STM32_MODER_SHIFT);
	otype = conf & (STM32_OTYPER_MASK << STM32_OTYPER_SHIFT);
	ospeed = conf & (STM32_OSPEEDR_MASK << STM32_OSPEEDR_SHIFT);
	pupd = conf & (STM32_PUPDR_MASK << STM32_PUPDR_SHIFT);

	LL_GPIO_SetPinOutputType(gpio, pin_ll, otype >> STM32_OTYPER_SHIFT);
	LL_GPIO_SetPinSpeed(gpio, pin_ll, ospeed >> STM32_OSPEEDR_SHIFT);
	LL_GPIO_SetPinPull(gpio, pin_ll, pupd >> STM32_PUPDR_SHIFT);
	if (mode == STM32_MODER_ALT_MODE) {
		if (pin < 8) {
			LL_GPIO_SetAFPin_0_7(gpio, pin_ll, func);
		} else {
			LL_GPIO_SetAFPin_8_15(gpio, pin_ll, func);
		}
	}
	LL_GPIO_SetPinMode(gpio, pin_ll, mode >> STM32_MODER_SHIFT);
}

static int stm32_port_get_raw(const struct device *port,
	gpio_port_value_t *value) {
	struct stm32_gpio *dev = to_stm32gpio(port);
	*value = LL_GPIO_ReadInputPort(dev->gpio);
	return 0;
}

static int stm32_port_set_masked_raw(const struct device *port,
	gpio_port_pins_t mask, gpio_port_value_t value) {
	struct stm32_gpio *dev = to_stm32gpio(port);
	uint32_t port_value;

	port_value = LL_GPIO_ReadOutputPort(dev->gpio);
	LL_GPIO_WriteOutputPort(dev->gpio, (port_value & ~mask) | (mask & value));
	return 0;
}

static int stm32_port_set_bits_raw(const struct device *port,
	gpio_port_pins_t pins) {
	struct stm32_gpio *dev = to_stm32gpio(port);

	LL_GPIO_SetOutputPin(dev->gpio, pins);
	return 0;
}

static int stm32_port_clear_bits_raw(const struct device *port,
	gpio_port_pins_t pins) {
	struct stm32_gpio *dev = to_stm32gpio(port);

	LL_GPIO_ResetOutputPin(dev->gpio, pins);
	return 0;
}

static int stm32_port_toggle_bits(const struct device *port,
	gpio_port_pins_t pins) {
	struct stm32_gpio *dev = to_stm32gpio(port);

	LL_GPIO_TogglePin(dev->gpio, pins);
	return 0;
}

static int stm32_pin_interrupt_configure(const struct device *port,
	gpio_pin_t pin, enum gpio_int_mode mode, enum gpio_int_trig trig) {
	struct stm32_gpio *dev = to_stm32gpio(port);
	bool rising_edge;
	bool falling_edge;

	if (mode != GPIO_INT_MODE_EDGE)
		return -ENOTSUP;

	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		falling_edge = true;
		rising_edge = false;
		break;
	case GPIO_INT_TRIG_HIGH:
		falling_edge = false;
		rising_edge = true;
		break;
	case GPIO_INT_TRIG_BOTH:
		falling_edge = true;
		rising_edge = true;
		break;
	default:
		return -ENOTSUP;
	}

	/* Configure gpio interrupt */
	stm32_set_exti_source(dev - stm32_gpio_port, pin);
	stm32_exti_trigger(pin, rising_edge, falling_edge);
	stm32_exti_enable(pin, true);

	return 0;
}

static int stm32_manage_callback(const struct device *port, struct gpio_callback *cb, 
	bool set) {
	struct gpio_callback *pos;
	int pin;

	if (cb->pin_mask == 0)
		return -EINVAL;

	pin = ffs(cb->pin_mask) - 1;
	if (set) {
		scoped_guard(os_irq) {
			STAILQ_FOREACH(pos, &exti_heads[pin], node) {
				if (pos == cb)
					return -EEXIST;
			}
			cb->port = port;
			STAILQ_INSERT_TAIL(&exti_heads[pin], cb, node);
		}
		return 0;
	}

	scoped_guard(os_irq) {
		STAILQ_FOREACH(pos, &exti_heads[pin], node) {
			if (pos == cb) {
				STAILQ_REMOVE(&exti_heads[pin], cb, gpio_callback, node);
				if (STAILQ_EMPTY(&exti_heads[pin])) 
					goto _rm_exti;
			}
		}
	}
	return -ENODATA;

_rm_exti:
	stm32_exti_enable(pin, false);
	stm32_exti_trigger(pin, false, false);
	return 0;
}

static int stm32_pin_configure(const struct device *port, gpio_pin_t pin,
	gpio_flags_t flags) {
	struct stm32_gpio *dev = to_stm32gpio(port);
	uint32_t pincfg;
	int err;

	/* figure out if we can map the requested GPIO
	 * configuration
	 */
	err = stm32_gpio_flags_to_conf(flags, &pincfg);
	if (err)
		return err;

	if ((flags & GPIO_OUTPUT) != 0) {
		if (flags & GPIO_OUTPUT_INIT_HIGH)
			stm32_port_set_bits_raw((struct device *)dev, BIT(pin));
		else if (flags & GPIO_OUTPUT_INIT_LOW)
			stm32_port_clear_bits_raw((struct device *)dev, BIT(pin));
	}

	gpio_stm32_configure_raw(dev, pin, pincfg, 0);
	return 0;
}

static int stm32_gpio_register_port(struct stm32_gpio *dev) {
	dev->dev.pin_configure = stm32_pin_configure;
#ifdef CONFIG_GPIO_GET_CONFIG
	dev->dev.pin_get_config = NULL;
#endif
	dev->dev.port_get_raw = stm32_port_get_raw;
	dev->dev.port_set_masked_raw = stm32_port_set_masked_raw;
	dev->dev.port_set_bits_raw = stm32_port_set_bits_raw;
	dev->dev.port_clear_bits_raw = stm32_port_clear_bits_raw;
	dev->dev.port_toggle_bits = stm32_port_toggle_bits;
	dev->dev.pin_interrupt_configure = stm32_pin_interrupt_configure;
	dev->dev.manage_callback = stm32_manage_callback;
#ifdef CONFIG_GPIO_GET_DIRECTION
	dev->dev.port_get_direction = NULL;
#endif
	return device_register((struct device *)dev);
}

static int stm32_gpio_setup(void) {
	for (size_t i = 0; i < rte_array_size(exti_heads); i++)
		STAILQ_INIT(&exti_heads[i]);

	for (size_t i = 0; i < rte_array_size(stm32_gpio_port); i++) {
		int err = stm32_gpio_register_port(stm32_gpio_port + i);
		if (err)
			return err;
	}

	for (size_t i = 0; i < rte_array_size(extiline_vec); i++) {
		void (*irqfn)(void *);

		if (i <= 4)
			irqfn = stm32_exti_isr;
		else
		 	irqfn = stm32_mulit_exti_isr;

		int err = request_irq(extiline_vec[i].irq, irqfn, &extiline_vec[i]);
		if (err) {
			pr_err("%s request irq(%d) failed(%d)\n", __func__,
				   extiline_vec[i].irq, err);
			return err;
		}
	}

	return gpio_dt_spec_init();
}

SYSINIT(stm32_gpio_setup, SI_PREDRIVER_LEVEL, 10);
