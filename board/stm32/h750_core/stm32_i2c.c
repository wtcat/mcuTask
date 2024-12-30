/*
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Ltd
 * Copryight (c) 2024 wtcat
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C Driver for: STM32F0, STM32F3, STM32F7, STM32L0, STM32L4, STM32WB and
 * STM32WL
 *
 */

#define TX_USE_BOARD_PRIVATE

#include "tx_api.h"
#include "drivers/i2c.h"


// #define CONFIG_I2C_TARGET
#define CONFIG_STM32_I2C1 

#define NSEC_PER_SEC 1000000000ul
#define LOG_ERR(fmt, ...) printk("[E]<i2c>: " fmt "\n", ##__VA_ARGS__)
#define LOG_INF(fmt, ...) printk("[I]<i2c>: " fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(...)

#define STM32_I2C_TRANSFER_TIMEOUT_MSEC 500

/* Use the algorithm to calcuate the I2C timing */
#ifndef STM32_I2C_VALID_TIMING_NBR
#define STM32_I2C_VALID_TIMING_NBR 128U
#endif
#define STM32_I2C_SPEED_FREQ_STANDARD 0U	   /* 100 kHz */
#define STM32_I2C_SPEED_FREQ_FAST 1U		   /* 400 kHz */
#define STM32_I2C_SPEED_FREQ_FAST_PLUS 2U	   /* 1 MHz */
#define STM32_I2C_ANALOG_FILTER_DELAY_MIN 50U  /* ns */
#define STM32_I2C_ANALOG_FILTER_DELAY_MAX 260U /* ns */
#define STM32_I2C_USE_ANALOG_FILTER 1U
#define STM32_I2C_DIGITAL_FILTER_COEF 0U
#define STM32_I2C_PRESC_MAX 16U
#define STM32_I2C_SCLDEL_MAX 16U
#define STM32_I2C_SDADEL_MAX 16U
#define STM32_I2C_SCLH_MAX 256U
#define STM32_I2C_SCLL_MAX 256U

/* I2C_DEVICE_Private_Types */
struct stm32_i2c_charac_t {
	uint32_t freq;		/* Frequency in Hz */
	uint32_t freq_min;	/* Minimum frequency in Hz */
	uint32_t freq_max;	/* Maximum frequency in Hz */
	uint32_t hddat_min; /* Minimum data hold time in ns */
	uint32_t vddat_max; /* Maximum data valid time in ns */
	uint32_t sudat_min; /* Minimum data setup time in ns */
	uint32_t lscl_min;	/* Minimum low period of the SCL clock in ns */
	uint32_t hscl_min;	/* Minimum high period of SCL clock in ns */
	uint32_t trise;		/* Rise time in ns */
	uint32_t tfall;		/* Fall time in ns */
	uint32_t dnf;		/* Digital noise filter coefficient */
};

struct stm32_i2c_timings_t {
	uint32_t presc;	  /* Timing prescaler */
	uint32_t tscldel; /* SCL delay */
	uint32_t tsdadel; /* SDA delay */
	uint32_t sclh;	  /* SCL high period */
	uint32_t scll;	  /* SCL low period */
};

/**
 * @brief structure to convey optional i2c timings settings
 */
struct i2c_config_timing {
	/* i2c peripheral clock in Hz */
	uint32_t periph_clock;
	/* i2c bus speed in Hz */
	uint32_t i2c_speed;
	/* I2C_TIMINGR register value of i2c v2 peripheral */
	uint32_t timing_setting;
};

struct stm32_i2c_config {
	int evt_irq;
	int err_irq;
	uint32_t clken;
	uint32_t bitrate;
};

struct stm32_i2c {
	struct i2c_device base;
	I2C_TypeDef *i2c;
	TX_SEMAPHORE device_sync_sem;
	TX_MUTEX bus_mutex;
	uint32_t dev_config;
	const struct stm32_i2c_config *config;

	/* Store the current timing structure set by runtime config */
	struct i2c_config_timing current_timing;
	struct {
		unsigned int is_write;
		unsigned int is_arlo;
		unsigned int is_nack;
		unsigned int is_err;
		struct i2c_msg *msg;
		unsigned int len;
		uint8_t *buf;
	} current;
#ifdef CONFIG_I2C_TARGET
	bool master_active;
	struct i2c_target_config *slave_cfg;
	struct i2c_target_config *slave2_cfg;
	bool slave_attached;
#endif
	bool is_configured;
	bool smbalert_active;
};

/* I2C_DEVICE Private Constants */
static const struct stm32_i2c_charac_t stm32_i2c_charac[] = {
	[STM32_I2C_SPEED_FREQ_STANDARD] = {
        .freq = 100000,
        .freq_min = 80000,
        .freq_max = 120000,
        .hddat_min = 0,
        .vddat_max = 3450,
        .sudat_min = 250,
        .lscl_min = 4700,
        .hscl_min = 4000,
        .trise = 640,
        .tfall = 20,
        .dnf = STM32_I2C_DIGITAL_FILTER_COEF,
    },
	[STM32_I2C_SPEED_FREQ_FAST] = {
        .freq = 400000,
        .freq_min = 320000,
        .freq_max = 480000,
        .hddat_min = 0,
        .vddat_max = 900,
        .sudat_min = 100,
        .lscl_min = 1300,
        .hscl_min = 600,
        .trise = 250,
        .tfall = 100,
        .dnf = STM32_I2C_DIGITAL_FILTER_COEF,
    },
	[STM32_I2C_SPEED_FREQ_FAST_PLUS] = {
        .freq = 1000000,
        .freq_min = 800000,
        .freq_max = 1200000,
        .hddat_min = 0,
        .vddat_max = 450,
        .sudat_min = 50,
        .lscl_min = 500,
        .hscl_min = 260,
        .trise = 60,
        .tfall = 100,
        .dnf = STM32_I2C_DIGITAL_FILTER_COEF,
    },
};

static struct stm32_i2c_timings_t i2c_valid_timing[STM32_I2C_VALID_TIMING_NBR];
static uint32_t i2c_valid_timing_nbr;

static int i2c_stm32_runtime_configure(struct device *dev, uint32_t config);

static uint32_t get_periph_clkrate(void) {
	return LL_RCC_GetI2CClockFreq(LL_RCC_I2C123_CLKSOURCE);
}

static inline uint32_t i2c_map_dt_bitrate(uint32_t bitrate) {
	switch (bitrate) {
	case I2C_BITRATE_STANDARD:
		return I2C_SPEED_STANDARD << I2C_SPEED_SHIFT;
	case I2C_BITRATE_FAST:
		return I2C_SPEED_FAST << I2C_SPEED_SHIFT;
	case I2C_BITRATE_FAST_PLUS:
		return I2C_SPEED_FAST_PLUS << I2C_SPEED_SHIFT;
	case I2C_BITRATE_HIGH:
		return I2C_SPEED_HIGH << I2C_SPEED_SHIFT;
	case I2C_BITRATE_ULTRA:
		return I2C_SPEED_ULTRA << I2C_SPEED_SHIFT;
	}
	return 0;
}

static inline void msg_init(struct stm32_i2c *dev, struct i2c_msg *msg,
							uint8_t *next_msg_flags, uint16_t slave, uint32_t transfer) {
	I2C_TypeDef *i2c = dev->i2c;

	if (LL_I2C_IsEnabledReloadMode(i2c)) {
		LL_I2C_SetTransferSize(i2c, msg->len);
	} else {
		if (I2C_ADDR_10_BITS & dev->dev_config) {
			LL_I2C_SetMasterAddressingMode(i2c, LL_I2C_ADDRESSING_MODE_10BIT);
			LL_I2C_SetSlaveAddr(i2c, (uint32_t)slave);
		} else {
			LL_I2C_SetMasterAddressingMode(i2c, LL_I2C_ADDRESSING_MODE_7BIT);
			LL_I2C_SetSlaveAddr(i2c, (uint32_t)slave << 1);
		}

		if (!(msg->flags & I2C_MSG_STOP) && next_msg_flags &&
			!(*next_msg_flags & I2C_MSG_RESTART)) {
			LL_I2C_EnableReloadMode(i2c);
		} else {
			LL_I2C_DisableReloadMode(i2c);
		}
		LL_I2C_DisableAutoEndMode(i2c);
		LL_I2C_SetTransferRequest(i2c, transfer);
		LL_I2C_SetTransferSize(i2c, msg->len);

#if defined(CONFIG_I2C_TARGET)
		dev->master_active = true;
#endif
		LL_I2C_Enable(i2c);
		LL_I2C_GenerateStartCondition(i2c);
	}
}

static void stm32_i2c_disable_transfer_interrupts(struct stm32_i2c *dev) {
	I2C_TypeDef *i2c = dev->i2c;

	LL_I2C_DisableIT_TX(i2c);
	LL_I2C_DisableIT_RX(i2c);
	LL_I2C_DisableIT_STOP(i2c);
	LL_I2C_DisableIT_NACK(i2c);
	LL_I2C_DisableIT_TC(i2c);

	if (!dev->smbalert_active) {
		LL_I2C_DisableIT_ERR(i2c);
	}
}

static void stm32_i2c_enable_transfer_interrupts(struct stm32_i2c *dev) {
	I2C_TypeDef *i2c = dev->i2c;

	LL_I2C_EnableIT_STOP(i2c);
	LL_I2C_EnableIT_NACK(i2c);
	LL_I2C_EnableIT_TC(i2c);
	LL_I2C_EnableIT_ERR(i2c);
}

static void stm32_i2c_master_mode_end(struct stm32_i2c *dev) {
	I2C_TypeDef *i2c = dev->i2c;

	stm32_i2c_disable_transfer_interrupts(dev);

	if (LL_I2C_IsEnabledReloadMode(i2c)) {
		LL_I2C_DisableReloadMode(i2c);
	}

#if defined(CONFIG_I2C_TARGET)
	dev->master_active = false;
	if (!dev->slave_attached && !dev->smbalert_active) {
		LL_I2C_Disable(i2c);
	}
#else
	if (!dev->smbalert_active) {
		LL_I2C_Disable(i2c);
	}
#endif
	tx_semaphore_put(&dev->device_sync_sem);
}

#if defined(CONFIG_I2C_TARGET)
static void stm32_i2c_slave_event(struct stm32_i2c *dev) {
	const struct stm32_i2c_config *cfg = dev->config;
	I2C_TypeDef *i2c = dev->i2c;
	const struct i2c_target_callbacks *slave_cb;
	struct i2c_target_config *slave_cfg;

	if (dev->slave_cfg->flags != I2C_TARGET_FLAGS_ADDR_10_BITS) {
		uint8_t slave_address;

		/* Choose the right slave from the address match code */
		slave_address = LL_I2C_GetAddressMatchCode(i2c) >> 1;
		if (dev->slave_cfg != NULL && slave_address == dev->slave_cfg->address) {
			slave_cfg = dev->slave_cfg;
		} else if (dev->slave2_cfg != NULL && slave_address == dev->slave2_cfg->address) {
			slave_cfg = dev->slave2_cfg;
		} else {
			rte_assert(0);
			return;
		}
	} else {
		/* On STM32 the LL_I2C_GetAddressMatchCode & (ISR register) returns
		 * only 7bits of address match so 10 bit dual addressing is broken.
		 * Revert to assuming single address match.
		 */
		if (dev->slave_cfg != NULL) {
			slave_cfg = dev->slave_cfg;
		} else {
			rte_assert(0);
			return;
		}
	}

	slave_cb = slave_cfg->callbacks;

	if (LL_I2C_IsActiveFlag_TXIS(i2c)) {
		uint8_t val;

		if (slave_cb->read_processed(slave_cfg, &val) < 0) {
			LOG_ERR("Error continuing reading\n");
		} else {
			LL_I2C_TransmitData8(i2c, val);
		}
		return;
	}

	if (LL_I2C_IsActiveFlag_RXNE(i2c)) {
		uint8_t val = LL_I2C_ReceiveData8(i2c);

		if (slave_cb->write_received(slave_cfg, val)) {
			LL_I2C_AcknowledgeNextData(i2c, LL_I2C_NACK);
		}
		return;
	}

	if (LL_I2C_IsActiveFlag_NACK(i2c)) {
		LL_I2C_ClearFlag_NACK(i2c);
	}

	if (LL_I2C_IsActiveFlag_STOP(i2c)) {
		stm32_i2c_disable_transfer_interrupts(dev);

		/* Flush remaining TX byte before clearing Stop Flag */
		LL_I2C_ClearFlag_TXE(i2c);

		LL_I2C_ClearFlag_STOP(i2c);

		slave_cb->stop(slave_cfg);

		/* Prepare to ACK next transmissions address byte */
		LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
	}

	if (LL_I2C_IsActiveFlag_ADDR(i2c)) {
		uint32_t dir;

		LL_I2C_ClearFlag_ADDR(i2c);

		dir = LL_I2C_GetTransferDirection(i2c);
		if (dir == LL_I2C_DIRECTION_WRITE) {
			if (slave_cb->write_requested(slave_cfg) < 0) {
				LOG_ERR("Error initiating writing");
			} else {
				LL_I2C_EnableIT_RX(i2c);
			}
		} else {
			uint8_t val;

			if (slave_cb->read_requested(slave_cfg, &val) < 0) {
				LOG_ERR("Error initiating reading");
			} else {
				LL_I2C_TransmitData8(i2c, val);
				LL_I2C_EnableIT_TX(i2c);
			}
		}

		stm32_i2c_enable_transfer_interrupts(dev);
	}
}

/* Attach and start I2C as target */
static int i2c_stm32_target_register(struct device *dev,
									 struct i2c_target_config *config) {
	struct stm32_i2c *data = (struct stm32_i2c *)dev;
	const struct stm32_i2c_config *cfg = data->config;

	I2C_TypeDef *i2c = data->i2c;
	uint32_t bitrate_cfg;
	int ret;

	if (!config) {
		return -EINVAL;
	}

	if (data->slave_cfg && data->slave2_cfg) {
		return -EBUSY;
	}

	if (data->master_active) {
		return -EBUSY;
	}

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);

	ret = i2c_stm32_runtime_configure(dev, bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("i2c: failure initializing");
		return ret;
	}

	LL_I2C_Enable(i2c);

	if (!data->slave_cfg) {
		data->slave_cfg = config;
		if (data->slave_cfg->flags == I2C_TARGET_FLAGS_ADDR_10_BITS) {
			LL_I2C_SetOwnAddress1(i2c, config->address, LL_I2C_OWNADDRESS1_10BIT);
			LOG_DBG("i2c: target #1 registered with 10-bit address");
		} else {
			LL_I2C_SetOwnAddress1(i2c, config->address << 1U, LL_I2C_OWNADDRESS1_7BIT);
			LOG_DBG("i2c: target #1 registered with 7-bit address");
		}

		LL_I2C_EnableOwnAddress1(i2c);

		LOG_DBG("i2c: target #1 registered");
	} else {
		data->slave2_cfg = config;

		if (data->slave2_cfg->flags == I2C_TARGET_FLAGS_ADDR_10_BITS) {
			return -EINVAL;
		}
		LL_I2C_SetOwnAddress2(i2c, config->address << 1U, LL_I2C_OWNADDRESS2_NOMASK);
		LL_I2C_EnableOwnAddress2(i2c);
		LOG_DBG("i2c: target #2 registered");
	}

	data->slave_attached = true;

	LL_I2C_EnableIT_ADDR(i2c);

	return 0;
}

static int i2c_stm32_target_unregister(struct device *dev,
									   struct i2c_target_config *config) {
	struct stm32_i2c *data = (struct stm32_i2c *)dev;
	const struct stm32_i2c_config *cfg = data->config;
	I2C_TypeDef *i2c = data->i2c;

	if (!data->slave_attached) {
		return -EINVAL;
	}

	if (data->master_active) {
		return -EBUSY;
	}

	if (config == data->slave_cfg) {
		LL_I2C_DisableOwnAddress1(i2c);
		data->slave_cfg = NULL;

		LOG_DBG("i2c: slave #1 unregistered");
	} else if (config == data->slave2_cfg) {
		LL_I2C_DisableOwnAddress2(i2c);
		data->slave2_cfg = NULL;

		LOG_DBG("i2c: slave #2 unregistered");
	} else {
		return -EINVAL;
	}

	/* Return if there is a slave remaining */
	if (data->slave_cfg || data->slave2_cfg) {
		LOG_DBG("i2c: target#%c still registered", data->slave_cfg ? '1' : '2');
		return 0;
	}

	/* Otherwise disable I2C */
	LL_I2C_DisableIT_ADDR(i2c);
	stm32_i2c_disable_transfer_interrupts(data);

	LL_I2C_ClearFlag_NACK(i2c);
	LL_I2C_ClearFlag_STOP(i2c);
	LL_I2C_ClearFlag_ADDR(i2c);

	if (!data->smbalert_active) {
		LL_I2C_Disable(i2c);
	}

	data->slave_attached = false;

	return 0;
}
#endif /* defined(CONFIG_I2C_TARGET) */

static void stm32_i2c_event(struct stm32_i2c *dev) {
	I2C_TypeDef *i2c = dev->i2c;

#if defined(CONFIG_I2C_TARGET)
	if (dev->slave_attached && !dev->master_active) {
		stm32_i2c_slave_event(dev);
		return;
	}
#endif
	if (dev->current.len) {
		/* Send next byte */
		if (LL_I2C_IsActiveFlag_TXIS(i2c)) {
			LL_I2C_TransmitData8(i2c, *dev->current.buf);
		}

		/* Receive next byte */
		if (LL_I2C_IsActiveFlag_RXNE(i2c)) {
			*dev->current.buf = LL_I2C_ReceiveData8(i2c);
		}

		dev->current.buf++;
		dev->current.len--;
	}

	/* NACK received */
	if (LL_I2C_IsActiveFlag_NACK(i2c)) {
		LL_I2C_ClearFlag_NACK(i2c);
		dev->current.is_nack = 1U;
		/*
		 * AutoEndMode is always disabled in master mode,
		 * so send a stop condition manually
		 */
		LL_I2C_GenerateStopCondition(i2c);
		return;
	}

	/* STOP received */
	if (LL_I2C_IsActiveFlag_STOP(i2c)) {
		LL_I2C_ClearFlag_STOP(i2c);
		LL_I2C_DisableReloadMode(i2c);
		goto end;
	}

	/* Transfer Complete or Transfer Complete Reload */
	if (LL_I2C_IsActiveFlag_TC(i2c) || LL_I2C_IsActiveFlag_TCR(i2c)) {
		/* Issue stop condition if necessary */
		if (dev->current.msg->flags & I2C_MSG_STOP) {
			LL_I2C_GenerateStopCondition(i2c);
		} else {
			stm32_i2c_disable_transfer_interrupts(dev);
			tx_semaphore_put(&dev->device_sync_sem);
		}
	}

	return;
end:
	stm32_i2c_master_mode_end(dev);
}

static int stm32_i2c_error(struct stm32_i2c *dev) {
	I2C_TypeDef *i2c = dev->i2c;

#if defined(CONFIG_I2C_TARGET)
	if (dev->slave_attached && !dev->master_active) {
		/* No need for a slave error function right now. */
		return 0;
	}
#endif

	if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
		LL_I2C_ClearFlag_ARLO(i2c);
		dev->current.is_arlo = 1U;
		goto end;
	}

	if (LL_I2C_IsActiveFlag_BERR(i2c)) {
		LL_I2C_ClearFlag_BERR(i2c);
		dev->current.is_err = 1U;
		goto end;
	}

	return 0;
end:
	stm32_i2c_master_mode_end(dev);
	return -EIO;
}

static void stm32_i2c_event_isr(void *arg) {
	struct stm32_i2c *dev = (struct stm32_i2c *)arg;

	stm32_i2c_event(dev);
}

static void stm32_i2c_error_isr(void *arg) {
	struct stm32_i2c *dev = (struct stm32_i2c *)arg;

	stm32_i2c_error(dev);
}

static int stm32_i2c_msg_write(struct stm32_i2c *dev, struct i2c_msg *msg,
							   uint8_t *next_msg_flags, uint16_t slave) {
	I2C_TypeDef *i2c = dev->i2c;
	bool is_timeout = false;

	dev->current.len = msg->len;
	dev->current.buf = msg->buf;
	dev->current.is_write = 1U;
	dev->current.is_nack = 0U;
	dev->current.is_err = 0U;
	dev->current.msg = msg;

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_WRITE);

	stm32_i2c_enable_transfer_interrupts(dev);
	LL_I2C_EnableIT_TX(i2c);

	if (tx_semaphore_get(&dev->device_sync_sem,
						 TX_MSEC(STM32_I2C_TRANSFER_TIMEOUT_MSEC)) != 0) {
		stm32_i2c_master_mode_end(dev);
		tx_semaphore_get(&dev->device_sync_sem, TX_WAIT_FOREVER);
		is_timeout = true;
	}

	if (dev->current.is_nack || dev->current.is_err || dev->current.is_arlo ||
		is_timeout) {
		goto error;
	}

	return 0;
error:
	if (dev->current.is_arlo) {
		LOG_DBG("%s: ARLO %d", __func__, dev->current.is_arlo);
		dev->current.is_arlo = 0U;
	}

	if (dev->current.is_nack) {
		LOG_DBG("%s: NACK", __func__);
		dev->current.is_nack = 0U;
	}

	if (dev->current.is_err) {
		LOG_DBG("%s: ERR %d", __func__, dev->current.is_err);
		dev->current.is_err = 0U;
	}

	if (is_timeout) {
		LOG_DBG("%s: TIMEOUT", __func__);
	}

	return -EIO;
}

static int stm32_i2c_msg_read(struct stm32_i2c *dev, struct i2c_msg *msg,
							  uint8_t *next_msg_flags, uint16_t slave) {
	struct stm32_i2c *data = dev;
	I2C_TypeDef *i2c = dev->i2c;
	bool is_timeout = false;

	data->current.len = msg->len;
	data->current.buf = msg->buf;
	data->current.is_write = 0U;
	data->current.is_arlo = 0U;
	data->current.is_err = 0U;
	data->current.is_nack = 0U;
	data->current.msg = msg;

	msg_init(dev, msg, next_msg_flags, slave, LL_I2C_REQUEST_READ);

	stm32_i2c_enable_transfer_interrupts(dev);
	LL_I2C_EnableIT_RX(i2c);

	if (tx_semaphore_get(&data->device_sync_sem,
						 TX_MSEC(STM32_I2C_TRANSFER_TIMEOUT_MSEC)) != 0) {
		stm32_i2c_master_mode_end(dev);
		tx_semaphore_get(&data->device_sync_sem, TX_WAIT_FOREVER);
		is_timeout = true;
	}

	if (data->current.is_nack || data->current.is_err || data->current.is_arlo ||
		is_timeout) {
		goto error;
	}

	return 0;
error:
	if (data->current.is_arlo) {
		LOG_DBG("%s: ARLO %d", __func__, data->current.is_arlo);
		data->current.is_arlo = 0U;
	}

	if (data->current.is_nack) {
		LOG_DBG("%s: NACK", __func__);
		data->current.is_nack = 0U;
	}

	if (data->current.is_err) {
		LOG_DBG("%s: ERR %d", __func__, data->current.is_err);
		data->current.is_err = 0U;
	}

	if (is_timeout) {
		LOG_DBG("%s: TIMEOUT", __func__);
	}

	return -EIO;
}

/*
 * Macro used to fix the compliance check warning :
 * "DEEP_INDENTATION: Too many leading tabs - consider code refactoring
 * in the i2c_compute_scll_sclh() function below
 */
#define I2C_LOOP_SCLH()                                                                  \
	;                                                                                    \
	if ((tscl >= clk_min) && (tscl <= clk_max) &&                                        \
		(tscl_h >= stm32_i2c_charac[i2c_speed].hscl_min) && (ti2cclk < tscl_h)) {        \
		int32_t error = (int32_t)tscl - (int32_t)ti2cspeed;                              \
                                                                                         \
		if (error < 0) {                                                                 \
			error = -error;                                                              \
		}                                                                                \
                                                                                         \
		if ((uint32_t)error < prev_error) {                                              \
			prev_error = (uint32_t)error;                                                \
			i2c_valid_timing[count].scll = scll;                                         \
			i2c_valid_timing[count].sclh = sclh;                                         \
			ret = count;                                                                 \
		}                                                                                \
	}

/*
 * @brief  Calculate SCLL and SCLH and find best configuration.
 * @param  clock_src_freq I2C source clock in Hz.
 * @param  i2c_speed I2C frequency (index).
 * @retval config index (0 to I2C_VALID_TIMING_NBR], 0xFFFFFFFF for no valid config.
 */
static uint32_t i2c_compute_scll_sclh(uint32_t clock_src_freq, uint32_t i2c_speed) {
	uint32_t ret = 0xFFFFFFFFU;
	uint32_t ti2cclk;
	uint32_t ti2cspeed;
	uint32_t prev_error;
	uint32_t dnf_delay;
	uint32_t clk_min, clk_max;
	uint32_t scll, sclh;
	uint32_t tafdel_min;

	ti2cclk = (NSEC_PER_SEC + (clock_src_freq / 2U)) / clock_src_freq;
	ti2cspeed = (NSEC_PER_SEC + (stm32_i2c_charac[i2c_speed].freq / 2U)) /
				stm32_i2c_charac[i2c_speed].freq;

	tafdel_min =
		(STM32_I2C_USE_ANALOG_FILTER == 1U) ? STM32_I2C_ANALOG_FILTER_DELAY_MIN : 0U;

	/* tDNF = DNF x tI2CCLK */
	dnf_delay = stm32_i2c_charac[i2c_speed].dnf * ti2cclk;

	clk_max = NSEC_PER_SEC / stm32_i2c_charac[i2c_speed].freq_min;
	clk_min = NSEC_PER_SEC / stm32_i2c_charac[i2c_speed].freq_max;

	prev_error = ti2cspeed;

	for (uint32_t count = 0; count < STM32_I2C_VALID_TIMING_NBR; count++) {
		/* tPRESC = (PRESC+1) x tI2CCLK*/
		uint32_t tpresc = (i2c_valid_timing[count].presc + 1U) * ti2cclk;

		for (scll = 0; scll < STM32_I2C_SCLL_MAX; scll++) {
			/* tLOW(min) <= tAF(min) + tDNF + 2 x tI2CCLK + [(SCLL+1) x tPRESC ] */
			uint32_t tscl_l =
				tafdel_min + dnf_delay + (2U * ti2cclk) + ((scll + 1U) * tpresc);

			/*
			 * The I2CCLK period tI2CCLK must respect the following conditions:
			 * tI2CCLK < (tLOW - tfilters) / 4 and tI2CCLK < tHIGH
			 */
			if ((tscl_l > stm32_i2c_charac[i2c_speed].lscl_min) &&
				(ti2cclk < ((tscl_l - tafdel_min - dnf_delay) / 4U))) {
				for (sclh = 0; sclh < STM32_I2C_SCLH_MAX; sclh++) {
					/*
					 * tHIGH(min) <= tAF(min) + tDNF +
					 * 2 x tI2CCLK + [(SCLH+1) x tPRESC]
					 */
					uint32_t tscl_h =
						tafdel_min + dnf_delay + (2U * ti2cclk) + ((sclh + 1U) * tpresc);

					/* tSCL = tf + tLOW + tr + tHIGH */
					uint32_t tscl = tscl_l + tscl_h + stm32_i2c_charac[i2c_speed].trise +
									stm32_i2c_charac[i2c_speed].tfall;

					/* get timings with the lowest clock error */
					I2C_LOOP_SCLH();
				}
			}
		}
	}

	return ret;
}

/*
 * Macro used to fix the compliance check warning :
 * "DEEP_INDENTATION: Too many leading tabs - consider code refactoring
 * in the i2c_compute_presc_scldel_sdadel() function below
 */
#define I2C_LOOP_SDADEL()                                                                \
	;                                                                                    \
                                                                                         \
	if ((tsdadel >= (uint32_t)tsdadel_min) && (tsdadel <= (uint32_t)tsdadel_max)) {      \
		if (presc != prev_presc) {                                                       \
			i2c_valid_timing[i2c_valid_timing_nbr].presc = presc;                        \
			i2c_valid_timing[i2c_valid_timing_nbr].tscldel = scldel;                     \
			i2c_valid_timing[i2c_valid_timing_nbr].tsdadel = sdadel;                     \
			prev_presc = presc;                                                          \
			i2c_valid_timing_nbr++;                                                      \
                                                                                         \
			if (i2c_valid_timing_nbr >= STM32_I2C_VALID_TIMING_NBR) {                    \
				break;                                                                   \
			}                                                                            \
		}                                                                                \
	}

/*
 * @brief  Compute PRESC, SCLDEL and SDADEL.
 * @param  clock_src_freq I2C source clock in Hz.
 * @param  i2c_speed I2C frequency (index).
 * @retval None.
 */
static void i2c_compute_presc_scldel_sdadel(uint32_t clock_src_freq, uint32_t i2c_speed) {
	uint32_t prev_presc = STM32_I2C_PRESC_MAX;
	uint32_t ti2cclk;
	int32_t tsdadel_min, tsdadel_max;
	int32_t tscldel_min;
	uint32_t presc, scldel, sdadel;
	uint32_t tafdel_min, tafdel_max;

	ti2cclk = (NSEC_PER_SEC + (clock_src_freq / 2U)) / clock_src_freq;

	tafdel_min =
		(STM32_I2C_USE_ANALOG_FILTER == 1U) ? STM32_I2C_ANALOG_FILTER_DELAY_MIN : 0U;
	tafdel_max =
		(STM32_I2C_USE_ANALOG_FILTER == 1U) ? STM32_I2C_ANALOG_FILTER_DELAY_MAX : 0U;
	/*
	 * tDNF = DNF x tI2CCLK
	 * tPRESC = (PRESC+1) x tI2CCLK
	 * SDADEL >= {tf +tHD;DAT(min) - tAF(min) - tDNF - [3 x tI2CCLK]} / {tPRESC}
	 * SDADEL <= {tVD;DAT(max) - tr - tAF(max) - tDNF- [4 x tI2CCLK]} / {tPRESC}
	 */
	tsdadel_min =
		(int32_t)stm32_i2c_charac[i2c_speed].tfall +
		(int32_t)stm32_i2c_charac[i2c_speed].hddat_min - (int32_t)tafdel_min -
		(int32_t)(((int32_t)stm32_i2c_charac[i2c_speed].dnf + 3) * (int32_t)ti2cclk);

	tsdadel_max =
		(int32_t)stm32_i2c_charac[i2c_speed].vddat_max -
		(int32_t)stm32_i2c_charac[i2c_speed].trise - (int32_t)tafdel_max -
		(int32_t)(((int32_t)stm32_i2c_charac[i2c_speed].dnf + 4) * (int32_t)ti2cclk);

	/* {[tr+ tSU;DAT(min)] / [tPRESC]} - 1 <= SCLDEL */
	tscldel_min = (int32_t)stm32_i2c_charac[i2c_speed].trise +
				  (int32_t)stm32_i2c_charac[i2c_speed].sudat_min;

	if (tsdadel_min <= 0) {
		tsdadel_min = 0;
	}

	if (tsdadel_max <= 0) {
		tsdadel_max = 0;
	}

	for (presc = 0; presc < STM32_I2C_PRESC_MAX; presc++) {
		for (scldel = 0; scldel < STM32_I2C_SCLDEL_MAX; scldel++) {
			/* TSCLDEL = (SCLDEL+1) * (PRESC+1) * TI2CCLK */
			uint32_t tscldel = (scldel + 1U) * (presc + 1U) * ti2cclk;

			if (tscldel >= (uint32_t)tscldel_min) {
				for (sdadel = 0; sdadel < STM32_I2C_SDADEL_MAX; sdadel++) {
					/* TSDADEL = SDADEL * (PRESC+1) * TI2CCLK */
					uint32_t tsdadel = (sdadel * (presc + 1U)) * ti2cclk;

					I2C_LOOP_SDADEL();
				}

				if (i2c_valid_timing_nbr >= STM32_I2C_VALID_TIMING_NBR) {
					return;
				}
			}
		}
	}
}

static int stm32_i2c_configure_timing(struct stm32_i2c *dev, uint32_t clock) {
	const struct stm32_i2c_config *cfg = dev->config;
	struct stm32_i2c *data = dev;
	I2C_TypeDef *i2c = data->i2c;
	uint32_t timing = 0U;
	uint32_t idx;
	uint32_t speed = 0U;
	uint32_t i2c_freq = cfg->bitrate;

	/* Reset valid timing count at the beginning of each new computation */
	i2c_valid_timing_nbr = 0;

	if ((clock != 0U) && (i2c_freq != 0U)) {
		for (speed = 0; speed <= (uint32_t)STM32_I2C_SPEED_FREQ_FAST_PLUS; speed++) {
			if ((i2c_freq >= stm32_i2c_charac[speed].freq_min) &&
				(i2c_freq <= stm32_i2c_charac[speed].freq_max)) {
				i2c_compute_presc_scldel_sdadel(clock, speed);
				idx = i2c_compute_scll_sclh(clock, speed);
				if (idx < STM32_I2C_VALID_TIMING_NBR) {
					timing = ((i2c_valid_timing[idx].presc & 0x0FU) << 28) |
							 ((i2c_valid_timing[idx].tscldel & 0x0FU) << 20) |
							 ((i2c_valid_timing[idx].tsdadel & 0x0FU) << 16) |
							 ((i2c_valid_timing[idx].sclh & 0xFFU) << 8) |
							 ((i2c_valid_timing[idx].scll & 0xFFU) << 0);
				}
				break;
			}
		}
	}

	/* Fill the current timing value in data structure at runtime */
	data->current_timing.periph_clock = clock;
	data->current_timing.i2c_speed = i2c_freq;
	data->current_timing.timing_setting = timing;

	LL_I2C_SetTiming(i2c, timing);

	return 0;
}

static int stm32_i2c_transaction(struct stm32_i2c *dev, struct i2c_msg msg,
								 uint8_t *next_msg_flags, uint16_t periph) {
	/*
	 * Perform a I2C transaction, while taking into account the STM32 I2C V2
	 * peripheral has a limited maximum chunk size. Take appropriate action
	 * if the message to send exceeds that limit.
	 *
	 * The last chunk of a transmission uses this function's next_msg_flags
	 * parameter for its backend calls (_write/_read). Any previous chunks
	 * use a copy of the current message's flags, with the STOP and RESTART
	 * bits turned off. This will cause the backend to use reload-mode,
	 * which will make the combination of all chunks to look like one big
	 * transaction on the wire.
	 */
	const uint32_t i2c_stm32_maxchunk = 255U;
	const uint8_t saved_flags = msg.flags;
	uint8_t combine_flags = saved_flags & ~(I2C_MSG_STOP | I2C_MSG_RESTART);
	uint8_t *flagsp = NULL;
	uint32_t rest = msg.len;
	int ret = 0;

	do { /* do ... while to allow zero-length transactions */
		if (msg.len > i2c_stm32_maxchunk) {
			msg.len = i2c_stm32_maxchunk;
			msg.flags &= ~I2C_MSG_STOP;
			flagsp = &combine_flags;
		} else {
			msg.flags = saved_flags;
			flagsp = next_msg_flags;
		}
		if ((msg.flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = stm32_i2c_msg_write(dev, &msg, flagsp, periph);
		} else {
			ret = stm32_i2c_msg_read(dev, &msg, flagsp, periph);
		}
		if (ret < 0) {
			break;
		}
		rest -= msg.len;
		msg.buf += msg.len;
		msg.len = rest;
	} while (rest > 0U);

	return ret;
}

static int i2c_stm32_get_config(struct device *dev, uint32_t *config) {
	struct stm32_i2c *data = (struct stm32_i2c *)dev;

	if (!data->is_configured) {
		LOG_ERR("I2C controller not configured");
		return -EIO;
	}

	*config = data->dev_config;

	/* Print the timing parameter of device data */
	LOG_INF("I2C timing value, report to the DTS :");

	/* I2C BIT RATE */
	if (data->current_timing.i2c_speed == 100000) {
		LOG_INF("timings = <%ld I2C_BITRATE_STANDARD 0x%lX>;",
				data->current_timing.periph_clock, data->current_timing.timing_setting);
	} else if (data->current_timing.i2c_speed == 400000) {
		LOG_INF("timings = <%ld I2C_BITRATE_FAST 0x%lX>;",
				data->current_timing.periph_clock, data->current_timing.timing_setting);
	} else if (data->current_timing.i2c_speed == 1000000) {
		LOG_INF("timings = <%ld I2C_SPEED_FAST_PLUS 0x%lX>;",
				data->current_timing.periph_clock, data->current_timing.timing_setting);
	}

	return 0;
}

static int i2c_stm32_runtime_configure(struct device *dev, uint32_t config) {
	struct stm32_i2c *data = (struct stm32_i2c *)dev;
	I2C_TypeDef *i2c = data->i2c;
	uint32_t i2c_clock = 0U;
	int ret;

	// TODO: get i2c clock-rate enable
	i2c_clock = get_periph_clkrate();

	data->dev_config = config;

	guard(os_mutex)(&data->bus_mutex);

	LL_I2C_Disable(i2c);
	ret = stm32_i2c_configure_timing(data, i2c_clock);

	if (data->smbalert_active) {
		LL_I2C_Enable(i2c);
	}

	return ret;
}

#define OPERATION(msg) (((struct i2c_msg *)msg)->flags & I2C_MSG_RW_MASK)

static int i2c_stm32_transfer(struct device *dev, struct i2c_msg *msg, uint8_t num_msgs,
							  uint16_t slave) {
	struct stm32_i2c *data = (struct stm32_i2c *)dev;
	struct i2c_msg *current, *next;
	int ret = 0;

	/* Check for validity of all messages, to prevent having to abort
	 * in the middle of a transfer
	 */
	current = msg;

	/*
	 * Set I2C_MSG_RESTART flag on first message in order to send start
	 * condition
	 */
	current->flags |= I2C_MSG_RESTART;

	for (uint8_t i = 1; i <= num_msgs; i++) {
		if (i < num_msgs) {
			next = current + 1;

			/*
			 * Restart condition between messages
			 * of different directions is required
			 */
			if (OPERATION(current) != OPERATION(next)) {
				if (!(next->flags & I2C_MSG_RESTART)) {
					ret = -EINVAL;
					break;
				}
			}

			/* Stop condition is only allowed on last message */
			if (current->flags & I2C_MSG_STOP) {
				ret = -EINVAL;
				break;
			}
		}

		current++;
	}

	if (ret) {
		return ret;
	}

	guard(os_mutex)(&data->bus_mutex);

	current = msg;
	while (num_msgs > 0) {
		uint8_t *next_msg_flags = NULL;

		if (num_msgs > 1) {
			next = current + 1;
			next_msg_flags = &(next->flags);
		}
		ret = stm32_i2c_transaction(data, *current, next_msg_flags, slave);
		if (ret < 0) {
			break;
		}
		current++;
		num_msgs--;
	}

	return ret;
}

static int stm32_i2cbus_init(struct stm32_i2c *dev) {
	const struct stm32_i2c_config *cfg = dev->config;
	uint32_t bitrate_cfg;
	int err;

	tx_semaphore_create(&dev->device_sync_sem, (CHAR *)dev->base.name, 0);
	err = request_irq(cfg->evt_irq, stm32_i2c_event_isr, dev);
	if (err) {
		LOG_ERR("failed to request irq(%d)\n", cfg->evt_irq);
		return err;
	}

	err = request_irq(cfg->err_irq, stm32_i2c_error_isr, dev);
	if (err) {
		LOG_ERR("failed to request irq(%d)\n", cfg->err_irq);
		return err;
	}

	dev->is_configured = false;

	/*
	 * initialize mutex used when multiple transfers
	 * are taking place to guarantee that each one is
	 * atomic and has exclusive access to the I2C bus.
	 */
	tx_mutex_create(&dev->bus_mutex, (CHAR *)dev->base.name, TX_INHERIT);

	/* Enable i2c clock */
	LL_APB1_GRP1_EnableClock(cfg->clken);

	bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);
	err = i2c_stm32_runtime_configure((struct device *)dev,
									  I2C_MODE_CONTROLLER | bitrate_cfg);
	if (err < 0) {
		LOG_ERR("i2c: failure initializing");
		return err;
	}

	dev->is_configured = true;
	err = device_register((struct device *)dev);
	if (err < 0) {
		LOG_ERR("failed to register device(%s)\n", dev->base.name);
		return err;
	}

	return 0;
}

#ifdef CONFIG_STM32_I2C1
static const struct stm32_i2c_config i2c1_config = {
    .evt_irq = I2C1_EV_IRQn,
    .err_irq = I2C1_ER_IRQn,
    .clken = LL_APB1_GRP1_PERIPH_I2C1,
    .bitrate = STM32_I2C_SPEED_FREQ_FAST
};

static struct stm32_i2c i2c1_bus = {
	.base = {
        .name = "i2c1",
        .dops = {
            .configure  = i2c_stm32_runtime_configure,
            .transfer   = i2c_stm32_transfer,
            .get_config = i2c_stm32_get_config,
    #ifdef CONFIG_I2C_TARGET
            .target_register = i2c_stm32_target_register,
            .target_unregister = i2c_stm32_target_unregister
    #endif
        }
    },
	.i2c = I2C1,
	.config = &i2c1_config
};
#endif /* CONFIG_STM32_I2C1 */

static int stm32_i2c_init(void) {
	int err = 0;
#ifdef CONFIG_STM32_I2C1
	err = stm32_i2cbus_init(&i2c1_bus);
#endif
	return err;
}

SYSINIT(stm32_i2c_init, SI_BUSDRIVER_LEVEL, 10);
