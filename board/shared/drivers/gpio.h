/*
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 * Copyright (c) 2019 Piotr Mienkowski
 * Copyright (c) 2017 ARM Ltd
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for GPIO drivers
 */

#ifndef DRIVERS_GPIO_H_
#define DRIVERS_GPIO_H_

#include <errno.h>
#include <sys/types.h>

#include <base/assert.h>
#include <base/sys/util_macro.h>
#include <drivers/device.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO Driver APIs
 * @defgroup gpio_interface GPIO Driver APIs
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */


/**
 * @brief GPIO Driver APIs
 * @defgroup gpio_interface GPIO Driver APIs
 * @ingroup io_interfaces
 * @{
 */

/** Mask for DT GPIO flags. */
#define GPIO_DT_FLAGS_MASK 0x3F

/**
 * @name GPIO pin active level flags
 * @{
 */

/** GPIO pin is active (has logical value '1') in low state. */
#define GPIO_ACTIVE_LOW         (1 << 0)
/** GPIO pin is active (has logical value '1') in high state. */
#define GPIO_ACTIVE_HIGH        (0 << 0)

/** @} */

/**
 * @name GPIO pin drive flags
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/* Configures GPIO output in single-ended mode (open drain or open source). */
#define GPIO_SINGLE_ENDED       (1 << 1)
/* Configures GPIO output in push-pull mode */
#define GPIO_PUSH_PULL          (0 << 1)

/* Indicates single ended open drain mode (wired AND). */
#define GPIO_LINE_OPEN_DRAIN    (1 << 2)
/* Indicates single ended open source mode (wired OR). */
#define GPIO_LINE_OPEN_SOURCE   (0 << 2)

/** @endcond */

/** Configures GPIO output in open drain mode (wired AND).
 *
 * @note 'Open Drain' mode also known as 'Open Collector' is an output
 * configuration which behaves like a switch that is either connected to ground
 * or disconnected.
 */
#define GPIO_OPEN_DRAIN         (GPIO_SINGLE_ENDED | GPIO_LINE_OPEN_DRAIN)
/** Configures GPIO output in open source mode (wired OR).
 *
 * @note 'Open Source' is a term used by software engineers to describe output
 * mode opposite to 'Open Drain'. It behaves like a switch that is either
 * connected to power supply or disconnected. There exist no corresponding
 * hardware schematic and the term is generally unknown to hardware engineers.
 */
#define GPIO_OPEN_SOURCE        (GPIO_SINGLE_ENDED | GPIO_LINE_OPEN_SOURCE)

/** @} */

/**
 * @name GPIO pin bias flags
 * @{
 */

/** Enables GPIO pin pull-up. */
#define GPIO_PULL_UP            (1 << 4)

/** Enable GPIO pin pull-down. */
#define GPIO_PULL_DOWN          (1 << 5)

/** @} */

/**
 * Configures GPIO interrupt to wakeup the system from low power mode.
 */
#define GPIO_INT_WAKEUP         (1 << 6)

/* Note: Bits 15 downto 8 are reserved for SoC specific flags. */

/**
 * @}
 */

/**
 * @name GPIO input/output configuration flags
 * @{
 */

/** Enables pin as input. */
#define GPIO_INPUT              (1U << 16)

/** Enables pin as output, no change to the output state. */
#define GPIO_OUTPUT             (1U << 17)

/** Disables pin for both input and output. */
#define GPIO_DISCONNECTED	0

/** @cond INTERNAL_HIDDEN */

/* Initializes output to a low state. */
#define GPIO_OUTPUT_INIT_LOW    (1U << 18)

/* Initializes output to a high state. */
#define GPIO_OUTPUT_INIT_HIGH   (1U << 19)

/* Initializes output based on logic level */
#define GPIO_OUTPUT_INIT_LOGICAL (1U << 20)

/** @endcond */

/** Configures GPIO pin as output and initializes it to a low state. */
#define GPIO_OUTPUT_LOW         (GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW)
/** Configures GPIO pin as output and initializes it to a high state. */
#define GPIO_OUTPUT_HIGH        (GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH)
/** Configures GPIO pin as output and initializes it to a logic 0. */
#define GPIO_OUTPUT_INACTIVE    (GPIO_OUTPUT |			\
				 GPIO_OUTPUT_INIT_LOW |		\
				 GPIO_OUTPUT_INIT_LOGICAL)
/** Configures GPIO pin as output and initializes it to a logic 1. */
#define GPIO_OUTPUT_ACTIVE      (GPIO_OUTPUT |			\
				 GPIO_OUTPUT_INIT_HIGH |	\
				 GPIO_OUTPUT_INIT_LOGICAL)

/** @} */

/**
 * @name GPIO interrupt configuration flags
 * The `GPIO_INT_*` flags are used to specify how input GPIO pins will trigger
 * interrupts. The interrupts can be sensitive to pin physical or logical level.
 * Interrupts sensitive to pin logical level take into account GPIO_ACTIVE_LOW
 * flag. If a pin was configured as Active Low, physical level low will be
 * considered as logical level 1 (an active state), physical level high will
 * be considered as logical level 0 (an inactive state).
 * The GPIO controller should reset the interrupt status, such as clearing the
 * pending bit, etc, when configuring the interrupt triggering properties.
 * Applications should use the `GPIO_INT_MODE_ENABLE_ONLY` and
 * `GPIO_INT_MODE_DISABLE_ONLY` flags to enable and disable interrupts on the
 * pin without changing any GPIO settings.
 * @{
 */

/** Disables GPIO pin interrupt. */
#define GPIO_INT_DISABLE               (1U << 21)

/** @cond INTERNAL_HIDDEN */

/* Enables GPIO pin interrupt. */
#define GPIO_INT_ENABLE                (1U << 22)

/* GPIO interrupt is sensitive to logical levels.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_LEVELS_LOGICAL        (1U << 23)

/* GPIO interrupt is edge sensitive.
 *
 * Note: by default interrupts are level sensitive.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_EDGE                  (1U << 24)

/* Trigger detection when input state is (or transitions to) physical low or
 * logical 0 level.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_LOW_0                 (1U << 25)

/* Trigger detection on input state is (or transitions to) physical high or
 * logical 1 level.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_HIGH_1                (1U << 26)

#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
/* Disable/Enable interrupt functionality without changing other interrupt
 * related register, such as clearing the pending register.
 *
 * This is a component flag that should be combined with `GPIO_INT_ENABLE` or
 * `GPIO_INT_DISABLE` flags to produce a meaningful configuration.
 */
#define GPIO_INT_ENABLE_DISABLE_ONLY   (1u << 27)
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

#define GPIO_INT_MASK                  (GPIO_INT_DISABLE | \
					GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_EDGE | \
					GPIO_INT_LOW_0 | \
					GPIO_INT_HIGH_1)

/** @endcond */

/** Configures GPIO interrupt to be triggered on pin rising edge and enables it.
 */
#define GPIO_INT_EDGE_RISING           (GPIO_INT_ENABLE | \
					GPIO_INT_EDGE | \
					GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin falling edge and enables
 * it.
 */
#define GPIO_INT_EDGE_FALLING          (GPIO_INT_ENABLE | \
					GPIO_INT_EDGE | \
					GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin rising or falling edge and
 * enables it.
 */
#define GPIO_INT_EDGE_BOTH             (GPIO_INT_ENABLE | \
					GPIO_INT_EDGE | \
					GPIO_INT_LOW_0 | \
					GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin physical level low and
 * enables it.
 */
#define GPIO_INT_LEVEL_LOW             (GPIO_INT_ENABLE | \
					GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin physical level high and
 * enables it.
 */
#define GPIO_INT_LEVEL_HIGH            (GPIO_INT_ENABLE | \
					GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin state change to logical
 * level 0 and enables it.
 */
#define GPIO_INT_EDGE_TO_INACTIVE      (GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_EDGE | \
					GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin state change to logical
 * level 1 and enables it.
 */
#define GPIO_INT_EDGE_TO_ACTIVE        (GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_EDGE | \
					GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin logical level 0 and enables
 * it.
 */
#define GPIO_INT_LEVEL_INACTIVE        (GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin logical level 1 and enables
 * it.
 */
#define GPIO_INT_LEVEL_ACTIVE          (GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_HIGH_1)

/** @} */

/** @cond INTERNAL_HIDDEN */
#define GPIO_DIR_MASK		(GPIO_INPUT | GPIO_OUTPUT)
/** @endcond */

/**
 * @brief Identifies a set of pins associated with a port.
 *
 * The pin with index n is present in the set if and only if the bit
 * identified by (1U << n) is set.
 */
typedef uint32_t gpio_port_pins_t;

/**
 * @brief Provides values for a set of pins associated with a port.
 *
 * The value for a pin with index n is high (physical mode) or active
 * (logical mode) if and only if the bit identified by (1U << n) is set.
 * Otherwise the value for the pin is low (physical mode) or inactive
 * (logical mode).
 *
 * Values of this type are often paired with a `gpio_port_pins_t` value
 * that specifies which encoded pin values are valid for the operation.
 */
typedef uint32_t gpio_port_value_t;

/**
 * @brief Provides a type to hold a GPIO pin index.
 *
 * This reduced-size type is sufficient to record a pin number,
 * e.g. from a devicetree GPIOS property.
 */
typedef uint8_t gpio_pin_t;

/**
 * @brief Provides a type to hold GPIO devicetree flags.
 *
 * All GPIO flags that can be expressed in devicetree fit in the low 16
 * bits of the full flags field, so use a reduced-size type to record
 * that part of a GPIOS property.
 *
 * The lower 8 bits are used for standard flags. The upper 8 bits are reserved
 * for SoC specific flags.
 */
typedef uint16_t gpio_dt_flags_t;

/**
 * @brief Provides a type to hold GPIO configuration flags.
 *
 * This type is sufficient to hold all flags used to control GPIO
 * configuration, whether pin or interrupt.
 */
typedef uint32_t gpio_flags_t;


/**
 * @brief Maximum number of pins that are supported by `gpio_port_pins_t`.
 */
#define GPIO_MAX_PINS_PER_PORT (sizeof(gpio_port_pins_t) * __CHAR_BIT__)

/**
 * This structure is common to all GPIO drivers and is expected to be
 * the first element in the object pointed to by the config field
 * in the device structure.
 */
struct gpio_driver_config {
	/** Mask identifying pins supported by the controller.
	 *
	 * Initialization of this mask is the responsibility of device
	 * instance generation in the driver.
	 */
	gpio_port_pins_t port_pin_mask;
};

/**
 * This structure is common to all GPIO drivers and is expected to be the first
 * element in the driver's struct driver_data declaration.
 */
struct gpio_driver_data {
	/** Mask identifying pins that are configured as active low.
	 *
	 * Management of this mask is the responsibility of the
	 * wrapper functions in this header.
	 */
	gpio_port_pins_t invert;
};

struct gpio_callback;

/**
 * @typedef gpio_callback_handler_t
 * @brief Define the application callback handler function signature
 *
 * @param cb Original struct gpio_callback owning this handler
 * @param pins Mask of pins that triggers the callback handler
 *
 * Note: cb pointer can be used to retrieve private data through
 * CONTAINER_OF() if original struct gpio_callback is stored in
 * another private structure.
 */
typedef void (*gpio_callback_handler_t)(struct gpio_callback *cb,
					gpio_port_pins_t pins);

/**
 * @brief GPIO callback structure
 *
 * Used to register a callback in the driver instance callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct gpio_callback.
 * Beware such structure should not be allocated on stack.
 *
 * Note: To help setting it, see gpio_init_callback() below
 */
struct gpio_callback {
	/** This is meant to be used in the driver and the user should not
	 * mess with it (see drivers/gpio/gpio_utils.h)
	 */
	STAILQ_ENTRY(gpio_callback) node;

	/* GPIO device port */
	const struct device *port;

	/** Actual callback function being called when relevant. */
	gpio_callback_handler_t handler;

	/** A mask of pins the callback is interested in, if 0 the callback
	 * will never be called. Such pin_mask can be modified whenever
	 * necessary by the owner, and thus will affect the handler being
	 * called or not. The selected pins must be configured to trigger
	 * an interrupt.
	 */
	gpio_port_pins_t pin_mask;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */

/* Used by driver api function pin_interrupt_configure, these are defined
 * in terms of the public flags so we can just mask and pass them
 * through to the driver api
 */
enum gpio_int_mode {
	GPIO_INT_MODE_DISABLED = GPIO_INT_DISABLE,
	GPIO_INT_MODE_LEVEL = GPIO_INT_ENABLE,
	GPIO_INT_MODE_EDGE = GPIO_INT_ENABLE | GPIO_INT_EDGE,
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	GPIO_INT_MODE_DISABLE_ONLY = GPIO_INT_DISABLE | GPIO_INT_ENABLE_DISABLE_ONLY,
	GPIO_INT_MODE_ENABLE_ONLY = GPIO_INT_ENABLE | GPIO_INT_ENABLE_DISABLE_ONLY,
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */
};

enum gpio_int_trig {
	/* Trigger detection when input state is (or transitions to)
	 * physical low. (Edge Falling or Active Low)
	 */
	GPIO_INT_TRIG_LOW = GPIO_INT_LOW_0,
	/* Trigger detection when input state is (or transitions to)
	 * physical high. (Edge Rising or Active High) */
	GPIO_INT_TRIG_HIGH = GPIO_INT_HIGH_1,
	/* Trigger detection on pin rising or falling edge. */
	GPIO_INT_TRIG_BOTH = GPIO_INT_LOW_0 | GPIO_INT_HIGH_1,
	/* Trigger a system wakeup. */
	GPIO_INT_TRIG_WAKE = GPIO_INT_WAKEUP,
};

DEVICE_CLASS_DEFINE(gpio_device,
#ifdef CONFIG_GPIO_INVERT_SUPPORT
	struct gpio_driver_data data;
#endif

	int (*pin_configure)(const struct device *port, gpio_pin_t pin,
			     gpio_flags_t flags);
#ifdef CONFIG_GPIO_GET_CONFIG
	int (*pin_get_config)(const struct device *port, gpio_pin_t pin,
			      gpio_flags_t *flags);
#endif
	int (*port_get_raw)(const struct device *port,
			    gpio_port_value_t *value);
	int (*port_set_masked_raw)(const struct device *port,
				   gpio_port_pins_t mask,
				   gpio_port_value_t value);
	int (*port_set_bits_raw)(const struct device *port,
				 gpio_port_pins_t pins);
	int (*port_clear_bits_raw)(const struct device *port,
				   gpio_port_pins_t pins);
	int (*port_toggle_bits)(const struct device *port,
				gpio_port_pins_t pins);
	int (*pin_interrupt_configure)(const struct device *port,
				       gpio_pin_t pin,
				       enum gpio_int_mode mode,
				       enum gpio_int_trig trig);
	int (*manage_callback)(const struct device *port,
			       struct gpio_callback *cb,
			       bool set);
	uint32_t (*get_pending_int)(const struct device *dev);
#ifdef CONFIG_GPIO_GET_DIRECTION
	int (*port_get_direction)(const struct device *port, gpio_port_pins_t map,
				  gpio_port_pins_t *inputs, gpio_port_pins_t *outputs);
#endif /* CONFIG_GPIO_GET_DIRECTION */
);

/**
 * @endcond
 */


/**
 * @brief Configure pin interrupt.
 *
 * @note This function can also be used to configure interrupts on pins
 *       not controlled directly by the GPIO module. That is, pins which are
 *       routed to other modules such as I2C, SPI, UART.
 *
 * @funcprops \isr_ok
 *
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number.
 * @param flags Interrupt configuration flags as defined by GPIO_INT_*.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If the operation is not implemented by the driver.
 * @retval -ENOTSUP If any of the configuration options is not supported
 *                  (unless otherwise directed by flag documentation).
 * @retval -EINVAL  Invalid argument.
 * @retval -EBUSY   Interrupt line required to configure pin interrupt is
 *                  already in use.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_interrupt_configure(const struct device *port,
						      gpio_pin_t pin,
						      gpio_flags_t flags)
{
	const struct gpio_device *api = (const struct gpio_device *)port;
#ifdef CONFIG_GPIO_INVERT_SUPPORT
	const struct gpio_driver_data *const data =
		(const struct gpio_driver_data *)&api->data;
#endif /* CONFIG_GPIO_INVERT_SUPPORT */
	enum gpio_int_trig trig;
	enum gpio_int_mode mode;

	if (api->pin_interrupt_configure == NULL) {
		return -ENOSYS;
	}

	rte_assert_msg((flags & (GPIO_INT_DISABLE | GPIO_INT_ENABLE))
		 != (GPIO_INT_DISABLE | GPIO_INT_ENABLE),
		 "Cannot both enable and disable interrupts");

	rte_assert_msg((flags & (GPIO_INT_DISABLE | GPIO_INT_ENABLE)) != 0U,
		 "Must either enable or disable interrupts");

	rte_assert_msg(((flags & GPIO_INT_ENABLE) == 0) ||
		 ((flags & GPIO_INT_EDGE) != 0) ||
		 ((flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)) !=
		  (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)),
		 "Only one of GPIO_INT_LOW_0, GPIO_INT_HIGH_1 can be "
		 "enabled for a level interrupt.");

	rte_assert_msg(((flags & GPIO_INT_ENABLE) == 0) ||
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
			 ((flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)) != 0) ||
			 (flags & GPIO_INT_ENABLE_DISABLE_ONLY) != 0,
#else
			 ((flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)) != 0),
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */
		 "At least one of GPIO_INT_LOW_0, GPIO_INT_HIGH_1 has to be "
		 "enabled.");

#ifdef CONFIG_GPIO_INVERT_SUPPORT
	if (((flags & GPIO_INT_LEVELS_LOGICAL) != 0) &&
	    ((data->invert & (gpio_port_pins_t)BIT(pin)) != 0)) {
		/* Invert signal bits */
		flags ^= (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1);
	}
#endif /* CONFIG_GPIO_INVERT_SUPPORT */

	trig = (enum gpio_int_trig)(flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1 | GPIO_INT_WAKEUP));
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	mode = (enum gpio_int_mode)(flags & (GPIO_INT_EDGE | GPIO_INT_DISABLE | GPIO_INT_ENABLE |
					     GPIO_INT_ENABLE_DISABLE_ONLY));
#else
	mode = (enum gpio_int_mode)(flags & (GPIO_INT_EDGE | GPIO_INT_DISABLE | GPIO_INT_ENABLE));
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

	return api->pin_interrupt_configure(port, pin, mode, trig);
}

/**
 * @brief Configure a single pin.
 *
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number to configure.
 * @param flags Flags for pin configuration: 'GPIO input/output configuration
 *        flags', 'GPIO pin drive flags', 'GPIO pin bias flags'.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if any of the configuration options is not supported
 *                  (unless otherwise directed by flag documentation).
 * @retval -EINVAL Invalid argument.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_configure(const struct device *port,
					    gpio_pin_t pin,
					    gpio_flags_t flags)
{
	const struct gpio_device *api = (const struct gpio_device *)port;
#ifdef CONFIG_GPIO_INVERT_SUPPORT
	struct gpio_driver_data *data = (struct gpio_driver_data *)&api->data;
#endif

	rte_assert_msg((flags & GPIO_INT_MASK) == 0,
		 "Interrupt flags are not supported");

	rte_assert_msg((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) !=
		 (GPIO_PULL_UP | GPIO_PULL_DOWN),
		 "Pull Up and Pull Down should not be enabled simultaneously");

	rte_assert_msg(!((flags & GPIO_INPUT) && !(flags & GPIO_OUTPUT) && (flags & GPIO_SINGLE_ENDED)),
		 "Input cannot be enabled for 'Open Drain', 'Open Source' modes without Output");

	rte_assert((flags & GPIO_SINGLE_ENDED) != 0 ||
			(flags & GPIO_LINE_OPEN_DRAIN) == 0);

	rte_assert_msg((flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH)) == 0
		 || (flags & GPIO_OUTPUT) != 0,
		 "Output needs to be enabled to be initialized low or high");

	rte_assert_msg((flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH))
		 != (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH),
		 "Output cannot be initialized low and high");

	if (((flags & GPIO_OUTPUT_INIT_LOGICAL) != 0)
	    && ((flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH)) != 0)
	    && ((flags & GPIO_ACTIVE_LOW) != 0)) {
		flags ^= GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH;
	}

	flags &= ~GPIO_OUTPUT_INIT_LOGICAL;

#ifdef CONFIG_GPIO_INVERT_SUPPORT
	if ((flags & GPIO_ACTIVE_LOW) != 0) {
		data->invert |= (gpio_port_pins_t)BIT(pin);
	} else {
		data->invert &= ~(gpio_port_pins_t)BIT(pin);
	}
#endif /* CONFIG_GPIO_INVERT_SUPPORT */

	return api->pin_configure(port, pin, flags);
}

/**
 * @brief Get direction of select pins in a port.
 *
 * Retrieve direction of each pin specified in @p map.
 *
 * If @p inputs or @p outputs is NULL, then this function does not get the
 * respective input or output direction information.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param map Bitmap of pin directions to query.
 * @param inputs Pointer to a variable where input directions will be stored.
 * @param outputs Pointer to a variable where output directions will be stored.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS if the underlying driver does not support this call.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
#ifdef CONFIG_GPIO_GET_DIRECTION
static inline int gpio_port_get_direction(const struct device *port, gpio_port_pins_t map,
						 gpio_port_pins_t *inputs,
						 gpio_port_pins_t *outputs)
{
	const struct gpio_device *api = (const struct gpio_device *)port;

	if (api->port_get_direction == NULL) {
		return -ENOSYS;
	}

	return api->port_get_direction(port, map, inputs, outputs);
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

/**
 * @brief Check if @p pin is configured for input
 *
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number to query the direction of
 *
 * @retval 1 if @p pin is configured as @ref GPIO_INPUT.
 * @retval 0 if @p pin is not configured as @ref GPIO_INPUT.
 * @retval -ENOSYS if the underlying driver does not support this call.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_is_input(const struct device *port, gpio_pin_t pin)
{
#ifdef CONFIG_GPIO_GET_DIRECTION
	gpio_port_pins_t pins;

	int rv = gpio_port_get_direction(port, BIT(pin), &pins, NULL);
	if (rv < 0) {
		return rv;
	}
	return (int)!!((gpio_port_pins_t)BIT(pin) & pins);
#else
	return -ENOSYS;
#endif /* CONFIG_GPIO_GET_DIRECTION */
}


/**
 * @brief Check if @p pin is configured for output
 *
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number to query the direction of
 *
 * @retval 1 if @p pin is configured as @ref GPIO_OUTPUT.
 * @retval 0 if @p pin is not configured as @ref GPIO_OUTPUT.
 * @retval -ENOSYS if the underlying driver does not support this call.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_is_output(const struct device *port, gpio_pin_t pin)
{
#ifdef CONFIG_GPIO_GET_DIRECTION
	gpio_port_pins_t pins;

	int rv = gpio_port_get_direction(port, BIT(pin), NULL, &pins);
	if (rv < 0) {
		return rv;
	}

	return (int)!!((gpio_port_pins_t)BIT(pin) & pins);
#else
	return -ENOSYS;
#endif /* CONFIG_GPIO_GET_DIRECTION */
}

/**
 * @brief Get a configuration of a single pin.
 *
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number which configuration is get.
 * @param flags Pointer to variable in which the current configuration will
 *              be stored if function is successful.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS if getting current pin configuration is not implemented
 *                  by the driver.
 * @retval -EINVAL Invalid argument.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */

#ifdef CONFIG_GPIO_GET_CONFIG
static inline int gpio_pin_get_config(const struct device *port,
					     gpio_pin_t pin,
					     gpio_flags_t *flags)
{
	const struct gpio_device *api =
		(const struct gpio_device *)port;

	if (api->pin_get_config == NULL) {
		return -ENOSYS;
	}

	return api->pin_get_config(port, pin, flags);
}
#endif

/**
 * @brief Get physical level of all input pins in a port.
 *
 * A low physical level on the pin will be interpreted as value 0. A high
 * physical level will be interpreted as value 1. This function ignores
 * GPIO_ACTIVE_LOW flag.
 *
 * Value of a pin with index n will be represented by bit n in the returned
 * port value.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param value Pointer to a variable where pin values will be stored.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	const struct gpio_device *api = (const struct gpio_device *)port;

	return api->port_get_raw(port, value);
}

/**
 * @brief Get logical level of all input pins in a port.
 *
 * Get logical level of an input pin taking into account GPIO_ACTIVE_LOW flag.
 * If pin is configured as Active High, a low physical level will be interpreted
 * as logical value 0. If pin is configured as Active Low, a low physical level
 * will be interpreted as logical value 1.
 *
 * Value of a pin with index n will be represented by bit n in the returned
 * port value.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param value Pointer to a variable where pin values will be stored.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_get(const struct device *port,
				gpio_port_value_t *value)
{
#ifdef CONFIG_GPIO_INVERT_SUPPORT
	const struct gpio_driver_data *const data =
			&((const struct gpio_device *)port)->data;
#endif
	int ret = gpio_port_get_raw(port, value);
#ifdef CONFIG_GPIO_INVERT_SUPPORT
	if (ret == 0) {
		*value ^= data->invert;
	}
#endif /* CONFIG_GPIO_INVERT_SUPPORT */
	return ret;
}

/**
 * @brief Set physical level of output pins in a port.
 *
 * Writing value 0 to the pin will set it to a low physical level. Writing
 * value 1 will set it to a high physical level. This function ignores
 * GPIO_ACTIVE_LOW flag.
 *
 * Pin with index n is represented by bit n in mask and value parameter.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param mask Mask indicating which pins will be modified.
 * @param value Value assigned to the output pins.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_set_masked_raw(const struct device *port,
						  gpio_port_pins_t mask,
						  gpio_port_value_t value)
{
	const struct gpio_device *api =
		(const struct gpio_device *)port;

	return api->port_set_masked_raw(port, mask, value);
}

/**
 * @brief Set logical level of output pins in a port.
 *
 * Set logical level of an output pin taking into account GPIO_ACTIVE_LOW flag.
 * Value 0 sets the pin in logical 0 / inactive state. Value 1 sets the pin in
 * logical 1 / active state. If pin is configured as Active High, the default,
 * setting it in inactive state will force the pin to a low physical level. If
 * pin is configured as Active Low, setting it in inactive state will force the
 * pin to a high physical level.
 *
 * Pin with index n is represented by bit n in mask and value parameter.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param mask Mask indicating which pins will be modified.
 * @param value Value assigned to the output pins.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_set_masked(const struct device *port,
				       gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
#ifdef CONFIG_GPIO_INVERT_SUPPORT
	const struct gpio_driver_data *data =
			&((const struct gpio_device *)port)->data;

	value ^= data->invert;
#endif /* CONFIG_GPIO_INVERT_SUPPORT */
	return gpio_port_set_masked_raw(port, mask, value);
}

/**
 * @brief Set physical level of selected output pins to high.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_set_bits_raw(const struct device *port,
						gpio_port_pins_t pins)
{
	const struct gpio_device *api =
		(const struct gpio_device *)port;

	return api->port_set_bits_raw(port, pins);
}

/**
 * @brief Set logical level of selected output pins to active.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_set_bits(const struct device *port,
				     gpio_port_pins_t pins)
{
	return gpio_port_set_masked(port, pins, pins);
}

/**
 * @brief Set physical level of selected output pins to low.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_clear_bits_raw(const struct device *port,
						  gpio_port_pins_t pins)
{
	const struct gpio_device *api =
		(const struct gpio_device *)port;

	return api->port_clear_bits_raw(port, pins);
}

/**
 * @brief Set logical level of selected output pins to inactive.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_clear_bits(const struct device *port,
				       gpio_port_pins_t pins)
{
	return gpio_port_set_masked(port, pins, 0);
}

/**
 * @brief Toggle level of selected output pins.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pins Value indicating which pins will be modified.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_toggle_bits(const struct device *port,
					       gpio_port_pins_t pins)
{
	const struct gpio_device *api =
		(const struct gpio_device *)port;

	return api->port_toggle_bits(port, pins);
}

/**
 * @brief Set physical level of selected output pins.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param set_pins Value indicating which pins will be set to high.
 * @param clear_pins Value indicating which pins will be set to low.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_set_clr_bits_raw(const struct device *port,
					     gpio_port_pins_t set_pins,
					     gpio_port_pins_t clear_pins)
{
	return gpio_port_set_masked_raw(port, set_pins | clear_pins, set_pins);
}

/**
 * @brief Set logical level of selected output pins.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param set_pins Value indicating which pins will be set to active.
 * @param clear_pins Value indicating which pins will be set to inactive.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_port_set_clr_bits(const struct device *port,
					 gpio_port_pins_t set_pins,
					 gpio_port_pins_t clear_pins)
{
	return gpio_port_set_masked(port, set_pins | clear_pins, set_pins);
}

/**
 * @brief Get physical level of an input pin.
 *
 * A low physical level on the pin will be interpreted as value 0. A high
 * physical level will be interpreted as value 1. This function ignores
 * GPIO_ACTIVE_LOW flag.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 *
 * @retval 1 If pin physical level is high.
 * @retval 0 If pin physical level is low.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_get_raw(const struct device *port, gpio_pin_t pin)
{
	gpio_port_value_t value;

	int ret = gpio_port_get_raw(port, &value);
	if (ret == 0) {
		ret = (value & (gpio_port_pins_t)BIT(pin)) != 0 ? 1 : 0;
	}

	return ret;
}

/**
 * @brief Get logical level of an input pin.
 *
 * Get logical level of an input pin taking into account GPIO_ACTIVE_LOW flag.
 * If pin is configured as Active High, a low physical level will be interpreted
 * as logical value 0. If pin is configured as Active Low, a low physical level
 * will be interpreted as logical value 1.
 *
 * Note: If pin is configured as Active High, the default, gpio_pin_get()
 *       function is equivalent to gpio_pin_get_raw().
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 *
 * @retval 1 If pin logical value is 1 / active.
 * @retval 0 If pin logical value is 0 / inactive.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_get(const struct device *port, gpio_pin_t pin)
{
	gpio_port_value_t value;
	int ret;

	ret = gpio_port_get(port, &value);
	if (ret == 0) {
		ret = (value & (gpio_port_pins_t)BIT(pin)) != 0 ? 1 : 0;
	}

	return ret;
}

/**
 * @brief Set physical level of an output pin.
 *
 * Writing value 0 to the pin will set it to a low physical level. Writing any
 * value other than 0 will set it to a high physical level. This function
 * ignores GPIO_ACTIVE_LOW flag.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 * @param value Value assigned to the pin.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_set_raw(const struct device *port, gpio_pin_t pin,
				   int value)
{
	if (value != 0)	{
		return gpio_port_set_bits_raw(port, (gpio_port_pins_t)BIT(pin));
	} else {
		return gpio_port_clear_bits_raw(port, (gpio_port_pins_t)BIT(pin));
	}
}

/**
 * @brief Set logical level of an output pin.
 *
 * Set logical level of an output pin taking into account GPIO_ACTIVE_LOW flag.
 * Value 0 sets the pin in logical 0 / inactive state. Any value other than 0
 * sets the pin in logical 1 / active state. If pin is configured as Active
 * High, the default, setting it in inactive state will force the pin to a low
 * physical level. If pin is configured as Active Low, setting it in inactive
 * state will force the pin to a high physical level.
 *
 * Note: If pin is configured as Active High, gpio_pin_set() function is
 *       equivalent to gpio_pin_set_raw().
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 * @param value Value assigned to the pin.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_set(const struct device *port, gpio_pin_t pin,
			       int value)
{
#ifdef CONFIG_GPIO_INVERT_SUPPORT
	const struct gpio_driver_data *const data =
			&((const struct gpio_device *)port)->data;

	if (data->invert & (gpio_port_pins_t)BIT(pin)) {
		value = (value != 0) ? 0 : 1;
	}
#endif /* CONFIG_GPIO_INVERT_SUPPORT */
	return gpio_pin_set_raw(port, pin, value);
}

/**
 * @brief Toggle pin level.
 *
 * @param port Pointer to the device structure for the driver instance.
 * @param pin Pin number.
 *
 * @retval 0 If successful.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
static inline int gpio_pin_toggle(const struct device *port, gpio_pin_t pin)
{
	return gpio_port_toggle_bits(port, (gpio_port_pins_t)BIT(pin));
}

/**
 * @brief Add an application callback.
 * @param port Pointer to the device structure for the driver instance.
 * @param callback A valid Application's callback structure pointer.
 * @retval 0 If successful
 * @retval -ENOSYS If driver does not implement the operation
 * @retval -errno Other negative errno code on failure.
 *
 * @note Callbacks may be added to the device from within a callback
 * handler invocation, but whether they are invoked for the current
 * GPIO event is not specified.
 *
 * Note: enables to add as many callback as needed on the same port.
 */
static inline int gpio_add_callback(const struct device *port,
				    struct gpio_callback *callback)
{
	const struct gpio_device *api = (const struct gpio_device *)port;

	if (api->manage_callback == NULL) {
		return -ENOSYS;
	}

	return api->manage_callback(port, callback, true);
}

/**
 * @brief Remove an application callback.
 * @param port Pointer to the device structure for the driver instance.
 * @param callback A valid application's callback structure pointer.
 * @retval 0 If successful
 * @retval -ENOSYS If driver does not implement the operation
 * @retval -errno Other negative errno code on failure.
 *
 * @warning It is explicitly permitted, within a callback handler, to
 * remove the registration for the callback that is running, i.e. @p
 * callback.  Attempts to remove other registrations on the same
 * device may result in undefined behavior, including failure to
 * invoke callbacks that remain registered and unintended invocation
 * of removed callbacks.
 *
 * Note: enables to remove as many callbacks as added through
 *       gpio_add_callback().
 */
static inline int gpio_remove_callback(const struct device *port,
				       struct gpio_callback *callback)
{
	const struct gpio_device *api = (const struct gpio_device *)port;

	if (api->manage_callback == NULL) {
		return -ENOSYS;
	}

	return api->manage_callback(port, callback, false);
}

/**
 * @brief Function to get pending interrupts
 *
 * The purpose of this function is to return the interrupt
 * status register for the device.
 * This is especially useful when waking up from
 * low power states to check the wake up source.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval status != 0 if at least one gpio interrupt is pending.
 * @retval 0 if no gpio interrupt is pending.
 * @retval -ENOSYS If driver does not implement the operation
 */
static inline int gpio_get_pending_int(const struct device *dev)
{
	const struct gpio_device *api = (const struct gpio_device *)dev;

	if (api->get_pending_int == NULL) {
		return -ENOSYS;
	}

	return api->get_pending_int(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif /* DRIVERS_GPIO_H_ */
