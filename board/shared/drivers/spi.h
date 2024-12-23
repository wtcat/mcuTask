/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for SPI drivers and applications
 */

#ifndef DRIVERS_SPI_H_
#define DRIVERS_SPI_H_

/**
 * @brief SPI Interface
 * @defgroup spi_interface SPI Interface
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include "drivers/device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name SPI operational mode
 * @{
 */
#define SPI_OP_MODE_MASTER	0U      /**< Master mode. */
#define SPI_OP_MODE_SLAVE	BIT(0)  /**< Slave mode. */
/** @cond INTERNAL_HIDDEN */
#define SPI_OP_MODE_MASK	0x1U
/** @endcond */
/** Get SPI operational mode. */
#define SPI_OP_MODE_GET(_operation_) ((_operation_) & SPI_OP_MODE_MASK)
/** @} */

/**
 * @name SPI Polarity & Phase Modes
 * @{
 */

/**
 * Clock Polarity: if set, clock idle state will be 1
 * and active state will be 0. If untouched, the inverse will be true
 * which is the default.
 */
#define SPI_MODE_CPOL		BIT(1)

/**
 * Clock Phase: this dictates when is the data captured, and depends
 * clock's polarity. When SPI_MODE_CPOL is set and this bit as well,
 * capture will occur on low to high transition and high to low if
 * this bit is not set (default). This is fully reversed if CPOL is
 * not set.
 */
#define SPI_MODE_CPHA		BIT(2)

/**
 * Whatever data is transmitted is looped-back to the receiving buffer of
 * the controller. This is fully controller dependent as some may not
 * support this, and can be used for testing purposes only.
 */
#define SPI_MODE_LOOP		BIT(3)
/** @cond INTERNAL_HIDDEN */
#define SPI_MODE_MASK		(0xEU)
/** @endcond */
/** Get SPI polarity and phase mode bits. */
#define SPI_MODE_GET(_mode_)			\
	((_mode_) & SPI_MODE_MASK)

/** @} */

/**
 * @name SPI Transfer modes (host controller dependent)
 * @{
 */
#define SPI_TRANSFER_MSB	(0U)    /**< Most significant bit first. */
#define SPI_TRANSFER_LSB	BIT(4)  /**< Least significant bit first. */
/** @} */

/**
 * @name SPI word size
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define SPI_WORD_SIZE_SHIFT	(5U)
#define SPI_WORD_SIZE_MASK	(0x3FU << SPI_WORD_SIZE_SHIFT)
/** @endcond */
/** Get SPI word size (data frame size) in bits. */
#define SPI_WORD_SIZE_GET(_operation_)					\
	(((_operation_) & SPI_WORD_SIZE_MASK) >> SPI_WORD_SIZE_SHIFT)
/** Set SPI word size (data frame size) in bits. */
#define SPI_WORD_SET(_word_size_)		\
	((_word_size_) << SPI_WORD_SIZE_SHIFT)
/** @} */

/**
 * @name Specific SPI devices control bits
 * @{
 */
/** Requests - if possible - to keep CS asserted after the transaction */
#define SPI_HOLD_ON_CS		BIT(12)
/** Keep the device locked after the transaction for the current config.
 * Use this with extreme caution (see spi_release() below) as it will
 * prevent other callers to access the SPI device until spi_release() is
 * properly called.
 */
#define SPI_LOCK_ON		BIT(13)

/** Active high logic on CS. Usually, and by default, CS logic is active
 * low. However, some devices may require the reverse logic: active high.
 * This bit will request the controller to use that logic. Note that not
 * all controllers are able to handle that natively. In this case deferring
 * the CS control to a gpio line through struct spi_cs_control would be
 * the solution.
 */
#define SPI_CS_ACTIVE_HIGH	BIT(14)
/** @} */

/**
 * @name SPI MISO lines
 * @{
 *
 * Some controllers support dual, quad or octal MISO lines connected to slaves.
 * Default is single, which is the case most of the time.
 * Without @kconfig{CONFIG_SPI_EXTENDED_MODES} being enabled, single is the
 * only supported one.
 */
#define SPI_LINES_SINGLE	(0U << 16)     /**< Single line */
#define SPI_LINES_DUAL		(1U << 16)     /**< Dual lines */
#define SPI_LINES_QUAD		(2U << 16)     /**< Quad lines */
#define SPI_LINES_OCTAL		(3U << 16)     /**< Octal lines */

#define SPI_LINES_MASK		(0x3U << 16)   /**< Mask for MISO lines in spi_operation_t */

/** @} */

/**
 * @brief SPI Chip Select control structure
 *
 * This can be used to control a CS line via a GPIO line, instead of
 * using the controller inner CS logic.
 *
 */
struct spi_cs_control {
	/**
	 * GPIO devicetree specification of CS GPIO.
	 * The device pointer can be set to NULL to fully inhibit CS control if
	 * necessary. The GPIO flags GPIO_ACTIVE_LOW/GPIO_ACTIVE_HIGH should be
	 * equivalent to SPI_CS_ACTIVE_HIGH/SPI_CS_ACTIVE_LOW options in struct
	 * spi_config.
	 */
#ifdef TX_DEVICE_SPI_CS_CONTROL_EXTENSION
    TX_DEVICE_SPI_CS_CONTROL_EXTENSION
#else
    uint32_t gpio;
#endif
	/**
	 * Delay in microseconds to wait before starting the
	 * transmission and before releasing the CS line.
	 */
	uint32_t delay;
};

/**
 * @brief SPI controller configuration structure
 */
struct spi_config {
	/** @brief Bus frequency in Hertz. */
	uint32_t frequency;
	/**
	 * @brief Operation flags.
	 *
	 * It is a bit field with the following parts:
	 *
	 * - 0:      Master or slave.
	 * - 1..3:   Polarity, phase and loop mode.
	 * - 4:      LSB or MSB first.
	 * - 5..10:  Size of a data frame (word) in bits.
	 * - 11:     Full/half duplex.
	 * - 12:     Hold on the CS line if possible.
	 * - 13:     Keep resource locked for the caller.
	 * - 14:     Active high CS logic.
	 * - 15:     Motorola or TI frame format (optional).
	 *
	 * If @kconfig{CONFIG_SPI_EXTENDED_MODES} is enabled:
	 *
	 * - 16..17: MISO lines (Single/Dual/Quad/Octal).
	 * - 18..31: Reserved for future use.
	 */
	uint32_t operation;

	/** @brief Slave number from 0 to host controller slave limit. */
	uint16_t slave;
	/**
	 * @brief GPIO chip-select line (optional, must be initialized to zero
	 * if not used).
	 */
	struct spi_cs_control cs;
};

/**
 * @brief Value that will never compare true with any valid overrun character
 */
#define SPI_MOSI_OVERRUN_UNKNOWN 0x100

/**
 * @brief SPI buffer structure
 */
struct spi_buf {
	/** Valid pointer to a data buffer, or NULL otherwise */
	void *buf;
	/** Length of the buffer @a buf in bytes.
	 * If @a buf is NULL, length which as to be sent as dummy bytes (as TX
	 * buffer) or the length of bytes that should be skipped (as RX buffer).
	 */
	size_t len;
};

/**
 * @brief SPI buffer array structure
 */
struct spi_buf_set {
	/** Pointer to an array of spi_buf, or NULL */
	const struct spi_buf *buffers;
	/** Length of the array (number of buffers) pointed by @a buffers */
	size_t count;
};

/**
 * @typedef spi_api_io
 * @brief Callback API for I/O
 * See spi_transceive() for argument descriptions
 */
typedef int (*spi_api_io)(struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs);

/**
 * @brief SPI callback for asynchronous transfer requests
 *
 * @param dev SPI device which is notifying of transfer completion or error
 * @param result Result code of the transfer request. 0 is success, -errno for failure.
 * @param data Transfer requester supplied data which is passed along to the callback.
 */
typedef void (*spi_callback_t)(struct device *dev, int result, void *data);

/**
 * @typedef spi_api_io
 * @brief Callback API for asynchronous I/O
 * See spi_transceive_signal() for argument descriptions
 */
typedef int (*spi_api_io_async)(struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs,
				spi_callback_t cb,
				void *userdata);

/**
 * @typedef spi_api_release
 * @brief Callback API for unlocking SPI device.
 * See spi_release() for argument descriptions
 */
typedef int (*spi_api_release)(struct device *dev,
			       const struct spi_config *config);


/**
 * @brief SPI driver API
 * This is the mandatory API any SPI driver needs to expose.
 */
struct spi_driver_api {
	spi_api_io transceive;
	spi_api_io_async transceive_async;
	spi_api_release release;
};

/*
 * SPI device declare
 */
DEVICE_CLASS_DEFINE(spi_device,
    struct spi_driver_api d_ops;
);

#define to_spi_devops(dev) dev_extension(dev, struct spi_driver_api)

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * @note This function is synchronous.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 *
 * @retval frames Positive number of frames received in slave mode.
 * @retval 0 If successful in master mode.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_transceive(struct device *dev,
					const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	const struct spi_driver_api *api = to_spi_devops(dev);

	return api->transceive(dev, config, tx_bufs, rx_bufs);
}

/**
 * @brief Read the specified amount of data from the SPI driver.
 *
 * @note This function is synchronous.
 *
 * @note This function is a helper function calling spi_transceive.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param rx_bufs Buffer array where data to be read will be written to.
 *
 * @retval frames Positive number of frames received in slave mode.
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_read(struct device *dev,
			   const struct spi_config *config,
			   const struct spi_buf_set *rx_bufs)
{
	return spi_transceive(dev, config, NULL, rx_bufs);
}

/**
 * @brief Write the specified amount of data from the SPI driver.
 *
 * @note This function is synchronous.
 *
 * @note This function is a helper function calling spi_transceive.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param tx_bufs Buffer array where data to be sent originates from.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_write(struct device *dev,
			    const struct spi_config *config,
			    const struct spi_buf_set *tx_bufs)
{
	return spi_transceive(dev, config, tx_bufs, NULL);
}

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * @note This function is asynchronous.
 *
 * @note This function is available only if @kconfig{CONFIG_SPI_ASYNC}
 * is selected.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 * @param callback Function pointer to completion callback.
 *	  (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 * @param userdata Userdata passed to callback
 *
 * @retval frames Positive number of frames received in slave mode.
 * @retval 0 If successful in master mode.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_transceive_cb(struct device *dev,
				    const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs,
				    spi_callback_t callback,
				    void *userdata)
{
	const struct spi_driver_api *api = to_spi_devops(dev);

	return api->transceive_async(dev, config, tx_bufs, rx_bufs, callback, userdata);
}

/**
 * @brief Release the SPI device locked on and/or the CS by the current config
 *
 * Note: This synchronous function is used to release either the lock on the
 *       SPI device and/or the CS line that was kept if, and if only,
 *       given config parameter was the last one to be used (in any of the
 *       above functions) and if it has the SPI_LOCK_ON bit set and/or the
 *       SPI_HOLD_ON_CS bit set into its operation bits field.
 *       This can be used if the caller needs to keep its hand on the SPI
 *       device for consecutive transactions and/or if it needs the device to
 *       stay selected. Usually both bits will be used along each other, so the
 *       the device is locked and stays on until another operation is necessary
 *       or until it gets released with the present function.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */

static inline int spi_release(struct device *dev,
				     const struct spi_config *config)
{
	const struct spi_driver_api *api = to_spi_devops(dev);

	return api->release(dev, config);
}

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_SPI_H_ */
