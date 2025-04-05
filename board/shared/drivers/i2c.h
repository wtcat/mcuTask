/**
 * @file
 *
 * @brief Public APIs for the I2C drivers.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef DRIVERS_I2C_H_
#define DRIVERS_I2C_H_

/**
 * @brief I2C Interface
 * @defgroup i2c_interface I2C Interface
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include "drivers/device.h"
#include "base/container/list.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The following #defines are used to configure the I2C controller.
 */
#define I2C_BITRATE_STANDARD	100000	/* 100 Kbit/s */
#define I2C_BITRATE_FAST	400000	/* 400 Kbit/s */
#define I2C_BITRATE_FAST_PLUS	1000000 /* 1 Mbit/s */
#define I2C_BITRATE_HIGH	3400000	/* 3.4 Mbit/s */
#define I2C_BITRATE_ULTRA	5000000 /* 5 Mbit/s */

/** I2C Standard Speed: 100 kHz */
#define I2C_SPEED_STANDARD		(0x1U)

/** I2C Fast Speed: 400 kHz */
#define I2C_SPEED_FAST			(0x2U)

/** I2C Fast Plus Speed: 1 MHz */
#define I2C_SPEED_FAST_PLUS		(0x3U)

/** I2C High Speed: 3.4 MHz */
#define I2C_SPEED_HIGH			(0x4U)

/** I2C Ultra Fast Speed: 5 MHz */
#define I2C_SPEED_ULTRA			(0x5U)

/** Device Tree specified speed */
#define I2C_SPEED_DT			(0x7U)

#define I2C_SPEED_SHIFT			(1U)
#define I2C_SPEED_SET(speed)		(((speed) << I2C_SPEED_SHIFT) \
						& I2C_SPEED_MASK)
#define I2C_SPEED_MASK			(0x7U << I2C_SPEED_SHIFT) /* 3 bits */
#define I2C_SPEED_GET(cfg) 		(((cfg) & I2C_SPEED_MASK) \
						>> I2C_SPEED_SHIFT)

/** Use 10-bit addressing. DEPRECATED - Use I2C_MSG_ADDR_10_BITS instead. */
#define I2C_ADDR_10_BITS		BIT(0)

/** Peripheral to act as Controller. */
#define I2C_MODE_CONTROLLER		BIT(4)

/**
 * @brief Complete I2C DT information
 *
 * @param bus is the I2C bus
 * @param addr is the target address
 */
struct i2c_dt_spec {
	struct device *bus;
	uint16_t addr;
};

/*
 * I2C_MSG_* are I2C Message flags.
 */

/** Write message to I2C bus. */
#define I2C_MSG_WRITE			(0U << 0U)

/** Read message from I2C bus. */
#define I2C_MSG_READ			BIT(0)

/** @cond INTERNAL_HIDDEN */
#define I2C_MSG_RW_MASK			BIT(0)
/** @endcond  */

/** Send STOP after this message. */
#define I2C_MSG_STOP			BIT(1)

/** RESTART I2C transaction for this message.
 *
 * @note Not all I2C drivers have or require explicit support for this
 * feature. Some drivers require this be present on a read message
 * that follows a write, or vice-versa.  Some drivers will merge
 * adjacent fragments into a single transaction using this flag; some
 * will not. */
#define I2C_MSG_RESTART			BIT(2)

/** Use 10-bit addressing for this message.
 *
 * @note Not all SoC I2C implementations support this feature. */
#define I2C_MSG_ADDR_10_BITS		BIT(3)

/**
 * @brief One I2C Message.
 *
 * This defines one I2C message to transact on the I2C bus.
 *
 * @note Some of the configurations supported by this API may not be
 * supported by specific SoC I2C hardware implementations, in
 * particular features related to bus transactions intended to read or
 * write data from different buffers within a single transaction.
 * Invocations of i2c_transfer() may not indicate an error when an
 * unsupported configuration is encountered.  In some cases drivers
 * will generate separate transactions for each message fragment, with
 * or without presence of @ref I2C_MSG_RESTART in #flags.
 */
struct i2c_msg {
	/** Data buffer in bytes */
	uint8_t		*buf;

	/** Length of buffer in bytes */
	uint32_t	len;

	/** Flags for this message */
	uint8_t		flags;
};

/**
 * @brief I2C callback for asynchronous transfer requests
 *
 * @param dev I2C device which is notifying of transfer completion or error
 * @param result Result code of the transfer request. 0 is success, -errno for failure.
 * @param data Transfer requester supplied data which is passed along to the callback.
 */
typedef void (*i2c_callback_t)(struct device *dev, int result, void *data);

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
struct i2c_target_config;

typedef int (*i2c_api_configure_t)(struct device *dev,
				   uint32_t dev_config);
typedef int (*i2c_api_get_config_t)(struct device *dev,
				    uint32_t *dev_config);
typedef int (*i2c_api_full_io_t)(struct device *dev,
				 struct i2c_msg *msgs,
				 uint8_t num_msgs,
				 uint16_t addr);
typedef int (*i2c_api_target_register_t)(struct device *dev,
					struct i2c_target_config *cfg);
typedef int (*i2c_api_target_unregister_t)(struct device *dev,
					  struct i2c_target_config *cfg);
typedef int (*i2c_api_transfer_cb_t)(struct device *dev,
				 struct i2c_msg *msgs,
				 uint8_t num_msgs,
				 uint16_t addr,
				 i2c_callback_t cb,
				 void *userdata);


typedef int (*i2c_api_recover_bus_t)(struct device *dev);

struct i2c_driver_api {
	i2c_api_configure_t configure;
	i2c_api_get_config_t get_config;
	i2c_api_full_io_t transfer;
	i2c_api_target_register_t target_register;
	i2c_api_target_unregister_t target_unregister;
	i2c_api_transfer_cb_t transfer_cb;
	i2c_api_recover_bus_t recover_bus;
};

typedef int (*i2c_target_api_register_t)(struct device *dev);
typedef int (*i2c_target_api_unregister_t)(struct device *dev);

struct i2c_target_driver_api {
	i2c_target_api_register_t driver_register;
	i2c_target_api_unregister_t driver_unregister;
};

/**
 * @endcond
 */

/** Target device responds to 10-bit addressing. */
#define I2C_TARGET_FLAGS_ADDR_10_BITS	BIT(0)

/** @brief Function called when a write to the device is initiated.
 *
 * This function is invoked by the controller when the bus completes a
 * start condition for a write operation to the address associated
 * with a particular device.
 *
 * A success return shall cause the controller to ACK the next byte
 * received.  An error return shall cause the controller to NACK the
 * next byte received.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @return 0 if the write is accepted, or a negative error code.
 */
typedef int (*i2c_target_write_requested_cb_t)(
		struct i2c_target_config *config);

/** @brief Function called when a write to the device is continued.
 *
 * This function is invoked by the controller when it completes
 * reception of a byte of data in an ongoing write operation to the
 * device.
 *
 * A success return shall cause the controller to ACK the next byte
 * received.  An error return shall cause the controller to NACK the
 * next byte received.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @param val the byte received by the controller.
 *
 * @return 0 if more data can be accepted, or a negative error
 * code.
 */
typedef int (*i2c_target_write_received_cb_t)(
		struct i2c_target_config *config, uint8_t val);

/** @brief Function called when a read from the device is initiated.
 *
 * This function is invoked by the controller when the bus completes a
 * start condition for a read operation from the address associated
 * with a particular device.
 *
 * The value returned in @p *val will be transmitted.  A success
 * return shall cause the controller to react to additional read
 * operations.  An error return shall cause the controller to ignore
 * bus operations until a new start condition is received.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @param val pointer to storage for the first byte of data to return
 * for the read request.
 *
 * @return 0 if more data can be requested, or a negative error code.
 */
typedef int (*i2c_target_read_requested_cb_t)(
		struct i2c_target_config *config, uint8_t *val);

/** @brief Function called when a read from the device is continued.
 *
 * This function is invoked by the controller when the bus is ready to
 * provide additional data for a read operation from the address
 * associated with the device device.
 *
 * The value returned in @p *val will be transmitted.  A success
 * return shall cause the controller to react to additional read
 * operations.  An error return shall cause the controller to ignore
 * bus operations until a new start condition is received.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @param val pointer to storage for the next byte of data to return
 * for the read request.
 *
 * @return 0 if data has been provided, or a negative error code.
 */
typedef int (*i2c_target_read_processed_cb_t)(
		struct i2c_target_config *config, uint8_t *val);

/** @brief Function called when a write to the device is completed.
 *
 * This function is invoked by the controller when it completes
 * reception of data from the source buffer to the destination
 * buffer in an ongoing write operation to the device.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @param ptr pointer to the buffer that contains the data to be transferred.
 *
 * @param len the length of the data to be transferred.
 */
typedef void (*i2c_target_buf_write_received_cb_t)(
		struct i2c_target_config *config, uint8_t *ptr, uint32_t len);

/** @brief Function called when a read from the device is initiated.
 *
 * This function is invoked by the controller when the bus is ready to
 * provide additional data by buffer for a read operation from the address
 * associated with the device.
 *
 * The value returned in @p **ptr and @p *len will be transmitted. A success
 * return shall cause the controller to react to additional read operations.
 * An error return shall cause the controller to ignore bus operations until
 * a new start condition is received.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @param ptr pointer to storage for the address of data buffer to return
 * for the read request.
 *
 * @param len pointer to storage for the length of the data to be transferred
 * for the read request.
 *
 * @return 0 if data has been provided, or a negative error code.
 */
typedef int (*i2c_target_buf_read_requested_cb_t)(
		struct i2c_target_config *config, uint8_t **ptr, uint32_t *len);


/** @brief Function called when a stop condition is observed after a
 * start condition addressed to a particular device.
 *
 * This function is invoked by the controller when the bus is ready to
 * provide additional data for a read operation from the address
 * associated with the device device.  After the function returns the
 * controller shall enter a state where it is ready to react to new
 * start conditions.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @return Ignored.
 */
typedef int (*i2c_target_stop_cb_t)(struct i2c_target_config *config);

/** @brief Structure providing callbacks to be implemented for devices
 * that supports the I2C target API.
 *
 * This structure may be shared by multiple devices that implement the
 * same API at different addresses on the bus.
 */
struct i2c_target_callbacks {
	i2c_target_write_requested_cb_t write_requested;
	i2c_target_read_requested_cb_t read_requested;
	i2c_target_write_received_cb_t write_received;
	i2c_target_read_processed_cb_t read_processed;
	i2c_target_buf_write_received_cb_t buf_write_received;
	i2c_target_buf_read_requested_cb_t buf_read_requested;
	i2c_target_stop_cb_t stop;
};

/** @brief Structure describing a device that supports the I2C
 * target API.
 *
 * Instances of this are passed to the i2c_target_register() and
 * i2c_target_unregister() functions to indicate addition and removal
 * of a target device, respective.
 *
 * Fields other than @c node must be initialized by the module that
 * implements the device behavior prior to passing the object
 * reference to i2c_target_register().
 */
struct i2c_target_config {
	/** Private, do not modify */
    struct rte_list node;

	/** Flags for the target device defined by I2C_TARGET_FLAGS_* constants */
	uint8_t flags;

	/** Address for this target device */
	uint16_t address;

	/** Callback functions */
	const struct i2c_target_callbacks *callbacks;
};

/**
 * @brief Check if the current message is a read operation
 *
 * @param msg The message to check
 * @return true if the I2C message is a read operation
 * @return false if the I2C message is a write operation
 */
static inline bool i2c_is_read_op(const struct i2c_msg *msg)
{
	return (msg->flags & I2C_MSG_READ) == I2C_MSG_READ;
}

/**
 * @brief Check if the current message includes a stop.
 *
 * @param msg The message to check
 * @return true if the I2C message includes a stop
 * @return false if the I2C message includes a stop
 */
static inline bool i2c_is_stop_op(const struct i2c_msg *msg)
{
	return (msg->flags & I2C_MSG_STOP) == I2C_MSG_STOP;
}

/*
 * I2C device declare 
 */
DEVICE_CLASS_DEFINE(i2c_device,
    struct i2c_driver_api dops;
);

DEVICE_CLASS_DEFINE(i2c_target_device,
    struct i2c_target_driver_api dops;
);

#define to_i2c_devops(dev) dev_extension(dev, void)

/**
 * @brief Configure operation of a host controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_config Bit-packed 32-bit value to the device runtime configuration
 * for the I2C controller.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 */
static inline int i2c_configure(struct device *dev,
				       uint32_t dev_config)
{
	const struct i2c_driver_api *api = to_i2c_devops(dev);

	return api->configure(dev, dev_config);
}

/**
 * @brief Get configuration of a host controller.
 *
 * This routine provides a way to get current configuration. It is allowed to
 * call the function before i2c_configure, because some I2C ports can be
 * configured during init process. However, if the I2C port is not configured,
 * i2c_get_config returns an error.
 *
 * i2c_get_config can return cached config or probe hardware, but it has to be
 * up to date with current configuration.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_config Pointer to return bit-packed 32-bit value of
 * the I2C controller configuration.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ERANGE Configured I2C frequency is invalid.
 * @retval -ENOSYS If get config is not implemented
 */
static inline int i2c_get_config(struct device *dev, uint32_t *dev_config)
{
	const struct i2c_driver_api *api = to_i2c_devops(dev);
	if (api->get_config == NULL)
		return -ENOSYS;

	return api->get_config(dev, dev_config);
}

/**
 * @brief Perform data transfer to another I2C device in controller mode.
 *
 * This routine provides a generic interface to perform data transfer
 * to another I2C device synchronously. Use i2c_read()/i2c_write()
 * for simple read or write.
 *
 * The array of message @a msgs must not be NULL.  The number of
 * message @a num_msgs may be zero,in which case no transfer occurs.
 *
 * @note Not all scatter/gather transactions can be supported by all
 * drivers.  As an example, a gather write (multiple consecutive
 * `i2c_msg` buffers all configured for `I2C_MSG_WRITE`) may be packed
 * into a single transaction by some drivers, but others may emit each
 * fragment as a distinct write transaction, which will not produce
 * the same behavior.  See the documentation of `struct i2c_msg` for
 * limitations on support for multi-message bus transactions.
 *
 * @note The last message in the scatter/gather transaction implies a STOP
 * whether or not it is explicitly set. This ensures the bus is in a good
 * state for the next transaction which may be from a different call context.
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in controller mode.
 * @param msgs Array of messages to transfer.
 * @param num_msgs Number of messages to transfer.
 * @param addr Address of the I2C target device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_transfer(struct device *dev,
				      struct i2c_msg *msgs, uint8_t num_msgs,
				      uint16_t addr)
{
	const struct i2c_driver_api *api = to_i2c_devops(dev);

	return api->transfer(dev, msgs, num_msgs, addr);
}

/**
 * @brief Perform data transfer to another I2C device in controller mode.
 *
 * This routine provides a generic interface to perform data transfer
 * to another I2C device asynchronously with a callback completion.
 *
 * @see i2c_transfer()
 * @funcprops \isr_ok
 *
 * @param dev Pointer to the device structure for an I2C controller
 *            driver configured in controller mode.
 * @param msgs Array of messages to transfer, must live until callback completes.
 * @param num_msgs Number of messages to transfer.
 * @param addr Address of the I2C target device.
 * @param cb Function pointer for completion callback.
 * @param userdata Userdata passed to callback.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If transfer async is not implemented
 * @retval -EWOULDBLOCK If the device is temporarily busy doing another transfer
 */
static inline int i2c_transfer_cb(struct device *dev,
				  struct i2c_msg *msgs,
				  uint8_t num_msgs,
				  uint16_t addr,
				  i2c_callback_t cb,
				  void *userdata)
{
	const struct i2c_driver_api *api = to_i2c_devops(dev);

	if (api->transfer_cb == NULL)
		return -ENOSYS;

	if (!num_msgs) {
		cb(dev, 0, userdata);
		return 0;
	}

	return api->transfer_cb(dev, msgs, num_msgs, addr, cb, userdata);
}

/**
 * @brief Write then read data from an I2C device asynchronously.
 *
 * This supports the common operation "this is what I want", "now give
 * it to me" transaction pair through a combined write-then-read bus
 * transaction but using i2c_transfer_cb. This helper function expects
 * caller to pass a message pointer with 2 and only 2 size.
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in master mode.
 * @param msgs Array of messages to transfer.
 * @param num_msgs Number of messages to transfer.
 * @param addr Address of the I2C device
 * @param write_buf Pointer to the data to be written
 * @param num_write Number of bytes to write
 * @param read_buf Pointer to storage for read data
 * @param num_read Number of bytes to read
 * @param cb Function pointer for completion callback.
 * @param userdata Userdata passed to callback.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */
static inline int i2c_write_read_cb(struct device *dev, struct i2c_msg *msgs,
				 uint8_t num_msgs, uint16_t addr, const void *write_buf,
				 size_t num_write, void *read_buf, size_t num_read,
				 i2c_callback_t cb, void *userdata)
{
	if ((msgs == NULL) || (num_msgs != 2)) {
		return -EINVAL;
	}

	msgs[0].buf = (uint8_t *)write_buf;
	msgs[0].len = num_write;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = (uint8_t *)read_buf;
	msgs[1].len = num_read;
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer_cb(dev, msgs, num_msgs, addr, cb, userdata);
}

/**
 * @brief Recover the I2C bus
 *
 * Attempt to recover the I2C bus.
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in controller mode.
 * @retval 0 If successful
 * @retval -EBUSY If bus is not clear after recovery attempt.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If bus recovery is not implemented
 */
static inline int i2c_recover_bus(struct device *dev)
{
	const struct i2c_driver_api *api = to_i2c_devops(dev);

	if (api->recover_bus == NULL)
		return -ENOSYS;

	return api->recover_bus(dev);
}

/**
 * @brief Registers the provided config as Target device of a controller.
 *
 * Enable I2C target mode for the 'dev' I2C bus driver using the provided
 * 'config' struct containing the functions and parameters to send bus
 * events. The I2C target will be registered at the address provided as 'address'
 * struct member. Addressing mode - 7 or 10 bit - depends on the 'flags'
 * struct member. Any I2C bus events related to the target mode will be passed
 * onto I2C target device driver via a set of callback functions provided in
 * the 'callbacks' struct member.
 *
 * Most of the existing hardware allows simultaneous support for controller
 * and target mode. This is however not guaranteed.
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in target mode.
 * @param cfg Config struct with functions and parameters used by the I2C driver
 * to send bus events
 *
 * @retval 0 Is successful
 * @retval -EINVAL If parameters are invalid
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If target mode is not implemented
 */
static inline int i2c_target_register(struct device *dev,
				     struct i2c_target_config *cfg)
{
	const struct i2c_driver_api *api = to_i2c_devops(dev);

	if (api->target_register == NULL)
		return -ENOSYS;

	return api->target_register(dev, cfg);
}

/**
 * @brief Unregisters the provided config as Target device
 *
 * This routine disables I2C target mode for the 'dev' I2C bus driver using
 * the provided 'config' struct containing the functions and parameters
 * to send bus events.
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in target mode.
 * @param cfg Config struct with functions and parameters used by the I2C driver
 * to send bus events
 *
 * @retval 0 Is successful
 * @retval -EINVAL If parameters are invalid
 * @retval -ENOSYS If target mode is not implemented
 */
static inline int i2c_target_unregister(struct device *dev,
				       struct i2c_target_config *cfg)
{
	const struct i2c_driver_api *api = to_i2c_devops(dev);

	if (api->target_unregister == NULL)
		return -ENOSYS;


	return api->target_unregister(dev, cfg);
}

/**
 * @brief Instructs the I2C Target device to register itself to the I2C Controller
 *
 * This routine instructs the I2C Target device to register itself to the I2C
 * Controller via its parent controller's i2c_target_register() API.
 *
 * @param dev Pointer to the device structure for the I2C target
 * device (not itself an I2C controller).
 *
 * @retval 0 Is successful
 * @retval -EINVAL If parameters are invalid
 * @retval -EIO General input / output error.
 */
static inline int i2c_target_driver_register(struct device *dev)
{
	const struct i2c_target_driver_api *api = to_i2c_devops(dev);

	return api->driver_register(dev);
}

/**
 * @brief Instructs the I2C Target device to unregister itself from the I2C
 * Controller
 *
 * This routine instructs the I2C Target device to unregister itself from the I2C
 * Controller via its parent controller's i2c_target_register() API.
 *
 * @param dev Pointer to the device structure for the I2C target
 * device (not itself an I2C controller).
 *
 * @retval 0 Is successful
 * @retval -EINVAL If parameters are invalid
 */
static inline int i2c_target_driver_unregister(struct device *dev)
{
	const struct i2c_target_driver_api *api = to_i2c_devops(dev);

	return api->driver_unregister(dev);
}

/*
 * Derived i2c APIs -- all implemented in terms of i2c_transfer()
 */

/**
 * @brief Write a set amount of data to an I2C device.
 *
 * This routine writes a set amount of data synchronously.
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in controller mode.
 * @param buf Memory pool from which the data is transferred.
 * @param num_bytes Number of bytes to write.
 * @param addr Address to the target I2C device for writing.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_write(struct device *dev, const uint8_t *buf,
			    uint32_t num_bytes, uint16_t addr)
{
	struct i2c_msg msg;

	msg.buf = (uint8_t *)buf;
	msg.len = num_bytes;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, &msg, 1, addr);
}

/**
 * @brief Read a set amount of data from an I2C device.
 *
 * This routine reads a set amount of data synchronously.
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in controller mode.
 * @param buf Memory pool that stores the retrieved data.
 * @param num_bytes Number of bytes to read.
 * @param addr Address of the I2C device being read.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_read(struct device *dev, uint8_t *buf,
			   uint32_t num_bytes, uint16_t addr)
{
	struct i2c_msg msg;

	msg.buf = buf;
	msg.len = num_bytes;
	msg.flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, &msg, 1, addr);
}

/**
 * @brief Write then read data from an I2C device.
 *
 * This supports the common operation "this is what I want", "now give
 * it to me" transaction pair through a combined write-then-read bus
 * transaction.
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in controller mode.
 * @param addr Address of the I2C device
 * @param write_buf Pointer to the data to be written
 * @param num_write Number of bytes to write
 * @param read_buf Pointer to storage for read data
 * @param num_read Number of bytes to read
 *
 * @retval 0 if successful
 * @retval negative on error.
 */
static inline int i2c_write_read(struct device *dev, uint16_t addr,
				 const void *write_buf, size_t num_write,
				 void *read_buf, size_t num_read)
{
	struct i2c_msg msg[2];

	msg[0].buf = (uint8_t *)write_buf;
	msg[0].len = num_write;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)read_buf;
	msg[1].len = num_read;
	msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msg, 2, addr);
}

/**
 * @brief Read multiple bytes from an internal address of an I2C device.
 *
 * This routine reads multiple bytes from an internal address of an
 * I2C device synchronously.
 *
 * Instances of this may be replaced by i2c_write_read().
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in controller mode.
 * @param dev_addr Address of the I2C device for reading.
 * @param start_addr Internal address from which the data is being read.
 * @param buf Memory pool that stores the retrieved data.
 * @param num_bytes Number of bytes being read.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_burst_read(struct device *dev,
				 uint16_t dev_addr,
				 uint8_t start_addr,
				 uint8_t *buf,
				 uint32_t num_bytes)
{
	return i2c_write_read(dev, dev_addr,
			      &start_addr, sizeof(start_addr),
			      buf, num_bytes);
}

/**
 * @brief Write multiple bytes to an internal address of an I2C device.
 *
 * This routine writes multiple bytes to an internal address of an
 * I2C device synchronously.
 *
 * @warning The combined write synthesized by this API may not be
 * supported on all I2C devices.  Uses of this API may be made more
 * portable by replacing them with calls to i2c_write() passing a
 * buffer containing the combined address and data.
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in controller mode.
 * @param dev_addr Address of the I2C device for writing.
 * @param start_addr Internal address to which the data is being written.
 * @param buf Memory pool from which the data is transferred.
 * @param num_bytes Number of bytes being written.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_burst_write(struct device *dev,
				  uint16_t dev_addr,
				  uint8_t start_addr,
				  const uint8_t *buf,
				  uint32_t num_bytes)
{
	struct i2c_msg msg[2];

	msg[0].buf = &start_addr;
	msg[0].len = 1U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, msg, 2, dev_addr);
}

/**
 * @brief Read internal register of an I2C device.
 *
 * This routine reads the value of an 8-bit internal register of an I2C
 * device synchronously.
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in controller mode.
 * @param dev_addr Address of the I2C device for reading.
 * @param reg_addr Address of the internal register being read.
 * @param value Memory pool that stores the retrieved register value.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_reg_read_byte(struct device *dev,
				    uint16_t dev_addr,
				    uint8_t reg_addr, uint8_t *value)
{
	return i2c_write_read(dev, dev_addr,
			      &reg_addr, sizeof(reg_addr),
			      value, sizeof(*value));
}

/**
 * @brief Write internal register of an I2C device.
 *
 * This routine writes a value to an 8-bit internal register of an I2C
 * device synchronously.
 *
 * @note This function internally combines the register and value into
 * a single bus transaction.
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in controller mode.
 * @param dev_addr Address of the I2C device for writing.
 * @param reg_addr Address of the internal register being written.
 * @param value Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_reg_write_byte(struct device *dev,
				     uint16_t dev_addr,
				     uint8_t reg_addr, uint8_t value)
{
	uint8_t tx_buf[2] = {reg_addr, value};

	return i2c_write(dev, tx_buf, 2, dev_addr);
}

/**
 * @brief Update internal register of an I2C device.
 *
 * This routine updates the value of a set of bits from an 8-bit internal
 * register of an I2C device synchronously.
 *
 * @note If the calculated new register value matches the value that
 * was read this function will not generate a write operation.
 *
 * @param dev Pointer to the device structure for an I2C controller
 * driver configured in controller mode.
 * @param dev_addr Address of the I2C device for updating.
 * @param reg_addr Address of the internal register being updated.
 * @param mask Bitmask for updating internal register.
 * @param value Value for updating internal register.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_reg_update_byte(struct device *dev,
				      uint8_t dev_addr,
				      uint8_t reg_addr, uint8_t mask,
				      uint8_t value)
{
	uint8_t old_value, new_value;
	int rc;

	rc = i2c_reg_read_byte(dev, dev_addr, reg_addr, &old_value);
	if (rc != 0) {
		return rc;
	}

	new_value = (old_value & ~mask) | (value & mask);
	if (new_value == old_value) {
		return 0;
	}

	return i2c_reg_write_byte(dev, dev_addr, reg_addr, new_value);
}

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_I2C_H_ */
