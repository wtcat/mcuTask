/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TX_USE_BOARD_PRIVATE

#include <tx_api.h>
#include <drivers/uart.h>
#include <base/assert.h>

#include "stm32_dma.h"

struct stm32_uart {
#define to_uart(_dev) ((struct stm32_uart *)(_dev))
	/* Must be place at first */
	struct uart_device class;
	USART_TypeDef *reg;
	struct stm32_dmachan txchan;
	struct stm32_dmachan rxchan;
	int irq;
	uint32_t clksrc;
	uint32_t clkbit;
    uint32_t de_enable: 1;
    uint32_t single_wire: 1;


    struct uart_config param;

    uart_irq_callback_user_data_t user_cb;
    void *user_data;

#if defined(CONFIG_UART_ASYNC_API)
    uart_callback_t async_cb;
    void *async_data;
#endif
};

#define UART_ID(_uart) (int)((_uart)->class.name[4] - '0')
#define __ASSERT(_c, _msg) rte_assert((_c))

/* Available everywhere except l1, f1, f2, f4. */
#ifdef USART_CR3_DEM
#define HAS_DRIVER_ENABLE 0//1
#else
#define HAS_DRIVER_ENABLE 0
#endif

static int 
uart_stm32_clkset(struct stm32_uart *uart, bool enable) {
    int devid = UART_ID(uart);

    if (devid == 1 || devid == 6) {
        if (enable)
            LL_APB2_GRP1_EnableClock(uart->clkbit);
        else
            LL_APB2_GRP1_DisableClock(uart->clkbit);
    } else {
        if (enable)
            LL_APB1_GRP1_EnableClock(uart->clkbit);
        else
            LL_APB1_GRP1_DisableClock(uart->clkbit);
    }
    return 0;
}

static inline void 
uart_stm32_set_baudrate(const struct device *dev, uint32_t baud_rate) {
	struct stm32_uart *uart = to_uart(dev);
	USART_TypeDef *usart = uart->reg;
	uint32_t clock_rate;

    clock_rate = LL_RCC_GetUSARTClockFreq(uart->clksrc);
#ifdef USART_CR1_OVER8
    LL_USART_SetOverSampling(usart, LL_USART_OVERSAMPLING_16);
#endif
    LL_USART_SetBaudRate(usart, clock_rate,
#ifdef USART_PRESC_PRESCALER
                            LL_USART_PRESCALER_DIV1,
#endif
#ifdef USART_CR1_OVER8
                            LL_USART_OVERSAMPLING_16,
#endif
                            baud_rate);
    /* Check BRR is greater than or equal to 16d */
    __ASSERT(LL_USART_ReadReg(usart, BRR) >= 16, "BaudRateReg >= 16");
}

static inline void 
uart_stm32_set_parity(const struct device *dev, uint32_t parity) {
	struct stm32_uart *uart = to_uart(dev);

	LL_USART_SetParity(uart->reg, parity);
}

static inline uint32_t 
uart_stm32_get_parity(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);

	return LL_USART_GetParity(uart->reg);
}

static inline void 
uart_stm32_set_stopbits(const struct device *dev, uint32_t stopbits) {
	struct stm32_uart *uart = to_uart(dev);

	LL_USART_SetStopBitsLength(uart->reg, stopbits);
}

static inline uint32_t 
uart_stm32_get_stopbits(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);

	return LL_USART_GetStopBitsLength(uart->reg);
}

static inline void 
uart_stm32_set_databits(const struct device *dev, uint32_t databits) {
	struct stm32_uart *uart = to_uart(dev);

	LL_USART_SetDataWidth(uart->reg, databits);
}

static inline uint32_t 
uart_stm32_get_databits(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);

	return LL_USART_GetDataWidth(uart->reg);
}

static inline void 
uart_stm32_set_hwctrl(const struct device *dev, uint32_t hwctrl) {
	struct stm32_uart *uart = to_uart(dev);

	LL_USART_SetHWFlowCtrl(uart->reg, hwctrl);
}

static inline uint32_t 
uart_stm32_get_hwctrl(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);

	return LL_USART_GetHWFlowCtrl(uart->reg);
}

#if HAS_DRIVER_ENABLE
static inline void 
uart_stm32_set_driver_enable(const struct device *dev, bool driver_enable) {
	struct stm32_uart *uart = to_uart(dev);

	if (driver_enable)
		LL_USART_EnableDEMode(uart->reg);
	else
		LL_USART_DisableDEMode(uart->reg);
}

static inline bool 
uart_stm32_get_driver_enable(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);

	return LL_USART_IsEnabledDEMode(uart->reg);
}
#endif

static inline uint32_t 
uart_stm32_cfg2ll_parity(enum uart_config_parity parity) {
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return LL_USART_PARITY_ODD;
	case UART_CFG_PARITY_EVEN:
		return LL_USART_PARITY_EVEN;
	case UART_CFG_PARITY_NONE:
	default:
		return LL_USART_PARITY_NONE;
	}
}

static inline enum uart_config_parity 
uart_stm32_ll2cfg_parity(uint32_t parity) {
	switch (parity) {
	case LL_USART_PARITY_ODD:
		return UART_CFG_PARITY_ODD;
	case LL_USART_PARITY_EVEN:
		return UART_CFG_PARITY_EVEN;
	case LL_USART_PARITY_NONE:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static inline uint32_t 
uart_stm32_cfg2ll_stopbits(struct stm32_uart *uart, enum uart_config_stop_bits sb) {
	switch (sb) {
/* Some MCU's don't support 0.5 stop bits */
#ifdef LL_USART_STOPBITS_0_5
	case UART_CFG_STOP_BITS_0_5:
#if HAS_LPUART
		if (IS_LPUART_INSTANCE(uart->reg)) {
			/* return the default */
			return LL_USART_STOPBITS_1;
		}
#endif /* HAS_LPUART */
		return LL_USART_STOPBITS_0_5;
#endif /* LL_USART_STOPBITS_0_5 */
	case UART_CFG_STOP_BITS_1:
		return LL_USART_STOPBITS_1;
/* Some MCU's don't support 1.5 stop bits */
#ifdef LL_USART_STOPBITS_1_5
	case UART_CFG_STOP_BITS_1_5:
#if HAS_LPUART
		if (IS_LPUART_INSTANCE(uart->reg)) {
			/* return the default */
			return LL_USART_STOPBITS_2;
		}
#endif
		return LL_USART_STOPBITS_1_5;
#endif /* LL_USART_STOPBITS_1_5 */
	case UART_CFG_STOP_BITS_2:
	default:
		return LL_USART_STOPBITS_2;
	}
}

static inline enum uart_config_stop_bits 
uart_stm32_ll2cfg_stopbits(uint32_t sb) {
	switch (sb) {
/* Some MCU's don't support 0.5 stop bits */
#ifdef LL_USART_STOPBITS_0_5
	case LL_USART_STOPBITS_0_5:
		return UART_CFG_STOP_BITS_0_5;
#endif /* LL_USART_STOPBITS_0_5 */
	case LL_USART_STOPBITS_1:
		return UART_CFG_STOP_BITS_1;
/* Some MCU's don't support 1.5 stop bits */
#ifdef LL_USART_STOPBITS_1_5
	case LL_USART_STOPBITS_1_5:
		return UART_CFG_STOP_BITS_1_5;
#endif /* LL_USART_STOPBITS_1_5 */
	case LL_USART_STOPBITS_2:
	default:
		return UART_CFG_STOP_BITS_2;
	}
}

static inline uint32_t 
uart_stm32_cfg2ll_databits(enum uart_config_data_bits db, enum uart_config_parity p) {
	switch (db) {
/* Some MCU's don't support 7B or 9B datawidth */
#ifdef LL_USART_DATAWIDTH_7B
	case UART_CFG_DATA_BITS_7:
		if (p == UART_CFG_PARITY_NONE) {
			return LL_USART_DATAWIDTH_7B;
		} else {
			return LL_USART_DATAWIDTH_8B;
		}
#endif /* LL_USART_DATAWIDTH_7B */
#ifdef LL_USART_DATAWIDTH_9B
	case UART_CFG_DATA_BITS_9:
		return LL_USART_DATAWIDTH_9B;
#endif /* LL_USART_DATAWIDTH_9B */
	case UART_CFG_DATA_BITS_8:
	default:
		if (p == UART_CFG_PARITY_NONE) {
			return LL_USART_DATAWIDTH_8B;
#ifdef LL_USART_DATAWIDTH_9B
		} else {
			return LL_USART_DATAWIDTH_9B;
#endif
		}
		return LL_USART_DATAWIDTH_8B;
	}
}

static inline enum uart_config_data_bits 
uart_stm32_ll2cfg_databits(uint32_t db, uint32_t p) {
	switch (db) {
/* Some MCU's don't support 7B or 9B datawidth */
#ifdef LL_USART_DATAWIDTH_7B
	case LL_USART_DATAWIDTH_7B:
		if (p == LL_USART_PARITY_NONE) {
			return UART_CFG_DATA_BITS_7;
		} else {
			return UART_CFG_DATA_BITS_6;
		}
#endif /* LL_USART_DATAWIDTH_7B */
#ifdef LL_USART_DATAWIDTH_9B
	case LL_USART_DATAWIDTH_9B:
		if (p == LL_USART_PARITY_NONE) {
			return UART_CFG_DATA_BITS_9;
		} else {
			return UART_CFG_DATA_BITS_8;
		}
#endif /* LL_USART_DATAWIDTH_9B */
	case LL_USART_DATAWIDTH_8B:
	default:
		if (p == LL_USART_PARITY_NONE) {
			return UART_CFG_DATA_BITS_8;
		} else {
			return UART_CFG_DATA_BITS_7;
		}
	}
}

/**
 * @brief  Get LL hardware flow control define from
 *         Zephyr hardware flow control option.
 * @note   Supports only UART_CFG_FLOW_CTRL_RTS_CTS and UART_CFG_FLOW_CTRL_RS485.
 * @param  fc: Zephyr hardware flow control option.
 * @retval LL_USART_HWCONTROL_RTS_CTS, or LL_USART_HWCONTROL_NONE.
 */
static inline uint32_t 
uart_stm32_cfg2ll_hwctrl(enum uart_config_flow_control fc) {
	if (fc == UART_CFG_FLOW_CTRL_RTS_CTS) {
		return LL_USART_HWCONTROL_RTS_CTS;
	} else if (fc == UART_CFG_FLOW_CTRL_RS485) {
		/* Driver Enable is handled separately */
		return LL_USART_HWCONTROL_NONE;
	}

	return LL_USART_HWCONTROL_NONE;
}

/**
 * @brief  Get Zephyr hardware flow control option from
 *         LL hardware flow control define.
 * @note   Supports only LL_USART_HWCONTROL_RTS_CTS.
 * @param  fc: LL hardware flow control definition.
 * @retval UART_CFG_FLOW_CTRL_RTS_CTS, or UART_CFG_FLOW_CTRL_NONE.
 */
static inline enum uart_config_flow_control 
uart_stm32_ll2cfg_hwctrl(uint32_t fc) {
	if (fc == LL_USART_HWCONTROL_RTS_CTS) {
		return UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	return UART_CFG_FLOW_CTRL_NONE;
}

static void uart_stm32_parameters_set(const struct device *dev,
									  const struct uart_config *cfg) {
	struct stm32_uart *uart = to_uart(dev);
	struct uart_config *uart_cfg = &uart->param;
	const uint32_t parity = uart_stm32_cfg2ll_parity(cfg->parity);
	const uint32_t stopbits = uart_stm32_cfg2ll_stopbits(uart, cfg->stop_bits);
	const uint32_t databits = uart_stm32_cfg2ll_databits(cfg->data_bits, cfg->parity);
	const uint32_t flowctrl = uart_stm32_cfg2ll_hwctrl(cfg->flow_ctrl);
#if HAS_DRIVER_ENABLE
	bool driver_enable = cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485;
#endif

	if (cfg == uart_cfg) {
		/* Called via (re-)init function, so the SoC either just booted,
		 * or is returning from a low-power state where it lost register
		 * contents
		 */
		LL_USART_ConfigCharacter(uart->reg, databits, parity, stopbits);
		uart_stm32_set_hwctrl(dev, flowctrl);
		uart_stm32_set_baudrate(dev, cfg->baudrate);
	} else {
		/* Called from application/subsys via uart_configure syscall */
		if (parity != uart_stm32_get_parity(dev)) {
			uart_stm32_set_parity(dev, parity);
		}

		if (stopbits != uart_stm32_get_stopbits(dev)) {
			uart_stm32_set_stopbits(dev, stopbits);
		}

		if (databits != uart_stm32_get_databits(dev)) {
			uart_stm32_set_databits(dev, databits);
		}

		if (flowctrl != uart_stm32_get_hwctrl(dev)) {
			uart_stm32_set_hwctrl(dev, flowctrl);
		}

#if HAS_DRIVER_ENABLE
		if (driver_enable != uart_stm32_get_driver_enable(dev)) {
			uart_stm32_set_driver_enable(dev, driver_enable);
		}
#endif

		if (cfg->baudrate != uart_cfg->baudrate) {
			uart_stm32_set_baudrate(dev, cfg->baudrate);
			uart_cfg->baudrate = cfg->baudrate;
		}
	}
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_stm32_configure(const struct device *dev, const struct uart_config *cfg) {
	struct stm32_uart *uart = to_uart(dev);
	USART_TypeDef *usart = uart->reg;
	struct uart_config *uart_cfg = &uart->param;
	const uint32_t parity = uart_stm32_cfg2ll_parity(cfg->parity);
	const uint32_t stopbits = uart_stm32_cfg2ll_stopbits(uart, cfg->stop_bits);
	const uint32_t databits = uart_stm32_cfg2ll_databits(cfg->data_bits, cfg->parity);

	/* Hardware doesn't support mark or space parity */
	if ((cfg->parity == UART_CFG_PARITY_MARK) || (cfg->parity == UART_CFG_PARITY_SPACE)) {
		return -ENOTSUP;
	}

	/* Driver does not supports parity + 9 databits */
	if ((cfg->parity != UART_CFG_PARITY_NONE) &&
		(cfg->data_bits == UART_CFG_DATA_BITS_9)) {
		return -ENOTSUP;
	}

	/* When the transformed ll stop bits don't match with what was requested, then it's
	 * not supported
	 */
	if (uart_stm32_ll2cfg_stopbits(stopbits) != cfg->stop_bits) {
		return -ENOTSUP;
	}

	/* When the transformed ll databits don't match with what was requested, then it's not
	 * supported
	 */
	if (uart_stm32_ll2cfg_databits(databits, parity) != cfg->data_bits) {
		return -ENOTSUP;
	}

	/* Driver supports only RTS/CTS and RS485 flow control */
	if (!(cfg->flow_ctrl == UART_CFG_FLOW_CTRL_NONE ||
		  (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS && IS_UART_HWFLOW_INSTANCE(usart))
#if HAS_DRIVER_ENABLE
		  || (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RS485 &&
			  IS_UART_DRIVER_ENABLE_INSTANCE(usart))
#endif
			  )) {
		return -ENOTSUP;
	}

	LL_USART_Disable(usart);

	/* Set basic parameters, such as data-/stop-bit, parity, and baudrate */
	uart_stm32_parameters_set(dev, cfg);

	LL_USART_Enable(usart);

	/* Upon successful configuration, persist the syscall-passed
	 * uart_config.
	 * This allows restoring it, should the device return from a low-power
	 * mode in which register contents are lost.
	 */
	*uart_cfg = *cfg;

	return 0;
};

static int uart_stm32_config_get(const struct device *dev, struct uart_config *cfg) {
	struct stm32_uart *uart = to_uart(dev);
	struct uart_config *uart_cfg = &uart->param;

	cfg->baudrate = uart_cfg->baudrate;
	cfg->parity = uart_stm32_ll2cfg_parity(uart_stm32_get_parity(dev));
	cfg->stop_bits = uart_stm32_ll2cfg_stopbits(uart_stm32_get_stopbits(dev));
	cfg->data_bits = uart_stm32_ll2cfg_databits(uart_stm32_get_databits(dev),
												uart_stm32_get_parity(dev));
	cfg->flow_ctrl = uart_stm32_ll2cfg_hwctrl(uart_stm32_get_hwctrl(dev));
#if HAS_DRIVER_ENABLE
	if (uart_stm32_get_driver_enable(dev)) {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_RS485;
	}
#endif
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_stm32_poll_in(const struct device *dev, unsigned char *c) {
	struct stm32_uart *uart = to_uart(dev);
	USART_TypeDef *usart = uart->reg;

	/* Clear overrun error flag */
	if (LL_USART_IsActiveFlag_ORE(usart)) {
		LL_USART_ClearFlag_ORE(usart);
	}

	/*
	 * On stm32 F4X, F1X, and F2X, the RXNE flag is affected (cleared) by
	 * the uart_err_check function call (on errors flags clearing)
	 */
	if (!LL_USART_IsActiveFlag_RXNE(usart)) {
		return -1;
	}

	*c = (unsigned char)LL_USART_ReceiveData8(usart);

	return 0;
}

static void uart_stm32_poll_out(const struct device *dev, unsigned char c) {
	struct stm32_uart *uart = to_uart(dev);
	USART_TypeDef *usart = uart->reg;

	/* Wait for TXE flag to be raised
	 * When TXE flag is raised, we lock interrupts to prevent interrupts (notably that of
	 * usart) or thread switch. Then, we can safely send our character. The character sent
	 * will be interlaced with the characters potentially send with interrupt transmission
	 * API
	 */
	do {
		if (LL_USART_IsActiveFlag_TXE(usart)) {
			scoped_guard(os_irq) {
				if (LL_USART_IsActiveFlag_TXE(usart)) {
					LL_USART_TransmitData8(usart, (uint8_t)c);
					break;
				}
			}
		}
	} while (!LL_USART_IsActiveFlag_TXE(usart));
}

static int uart_stm32_err_check(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);
	USART_TypeDef *usart = uart->reg;
	uint32_t err = 0U;
#define ISR_ERR (USART_ISR_ORE | USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_LBDF)
	/* Check for errors, then clear them.
	 * Some SoC clear all error flags when at least
	 * one is cleared. (e.g. F4X, F1X, and F2X).
	 * The stm32 F4X, F1X, and F2X also reads the usart DR when clearing Errors
	 */
    
    uint32_t sr = usart->ISR;

    usart->ICR = sr & ISR_ERR;
	if (sr & USART_ISR_ORE) 
		err |= UART_ERROR_OVERRUN;
	
	if (sr & USART_ISR_PE) 
		err |= UART_ERROR_PARITY;

	if (sr & USART_ISR_FE) 
		err |= UART_ERROR_FRAMING;

	if (sr & USART_ISR_NE) 
		err |= UART_ERROR_NOISE;
	
	if (sr & USART_ISR_LBDF) 
		err |= UART_BREAK;
	
	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int 
uart_stm32_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size) {
	struct stm32_uart *uart = to_uart(dev);
	USART_TypeDef *usart = uart->reg;
	int num_tx = 0U;

	if (!LL_USART_IsActiveFlag_TXE(usart))
		return num_tx;

	/* Lock interrupts to prevent nested interrupts or thread switch */
	scoped_guard(os_irq) {
		while ((size - num_tx > 0) && LL_USART_IsActiveFlag_TXE(usart)) {
			/* TXE flag will be cleared with byte write to DR|RDR register */
			/* Send a character (8bit) */
			LL_USART_TransmitData8(usart, tx_data[num_tx]);
			num_tx++;
		}
	}

	return num_tx;
}

static int 
uart_stm32_fifo_read(const struct device *dev, uint8_t *rx_data, const int size) {
	struct stm32_uart *uart = to_uart(dev);
	USART_TypeDef *usart = uart->reg;
	int num_rx = 0U;

	while ((size - num_rx > 0) && LL_USART_IsActiveFlag_RXNE(usart)) {
		/* RXNE flag will be cleared upon read from DR|RDR register */

		rx_data[num_rx] = LL_USART_ReceiveData8(usart);
		num_rx++;

		/* Clear overrun error flag */
		if (LL_USART_IsActiveFlag_ORE(usart)) {
			LL_USART_ClearFlag_ORE(usart);
			/*
			 * On stm32 F4X, F1X, and F2X, the RXNE flag is affected (cleared) by
			 * the uart_err_check function call (on errors flags clearing)
			 */
		}
	}

	return num_rx;
}

static void uart_stm32_irq_tx_enable(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);

	LL_USART_EnableIT_TC(uart->reg);
}

static void uart_stm32_irq_tx_disable(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);

	LL_USART_DisableIT_TC(uart->reg);
}

static int uart_stm32_irq_tx_ready(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);

	return LL_USART_IsActiveFlag_TXE(uart->reg) &&
		   LL_USART_IsEnabledIT_TC(uart->reg);
}

static int uart_stm32_irq_tx_complete(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);

	return LL_USART_IsActiveFlag_TC(uart->reg);
}

static void uart_stm32_irq_rx_enable(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);

	LL_USART_EnableIT_RXNE(uart->reg);
}

static void uart_stm32_irq_rx_disable(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);

	LL_USART_DisableIT_RXNE(uart->reg);
}

static int uart_stm32_irq_rx_ready(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);
	/*
	 * On stm32 F4X, F1X, and F2X, the RXNE flag is affected (cleared) by
	 * the uart_err_check function call (on errors flags clearing)
	 */
	return LL_USART_IsActiveFlag_RXNE(uart->reg);
}

static void uart_stm32_irq_err_enable(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);
	USART_TypeDef *usart = uart->reg;

	/* Enable FE, ORE interruptions */
	LL_USART_EnableIT_ERROR(usart);

	/* Enable Line break detection */
	if (IS_UART_LIN_INSTANCE(usart)) {
		LL_USART_EnableIT_LBD(usart);
	}

	/* Enable parity error interruption */
	LL_USART_EnableIT_PE(usart);
}

static void uart_stm32_irq_err_disable(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);
	USART_TypeDef *usart = uart->reg;

	/* Disable FE, ORE interruptions */
	LL_USART_DisableIT_ERROR(usart);
#if !defined(CONFIG_SOC_SERIES_STM32F0X) || defined(USART_LIN_SUPPORT)
	/* Disable Line break detection */
	if (IS_UART_LIN_INSTANCE(usart)) {
		LL_USART_DisableIT_LBD(usart);
	}
#endif
	/* Disable parity error interruption */
	LL_USART_DisableIT_PE(usart);
}

static int uart_stm32_irq_is_pending(const struct device *dev) {
	USART_TypeDef *usart =  to_uart(dev)->reg;
	uint32_t sr = usart->ISR;
	uint32_t cr1 = usart->CR1;

	return (((sr & USART_ISR_RXNE_RXFNE) && (cr1 & USART_CR1_RXNEIE_RXFNEIE)) ||
			((sr & USART_ISR_TC) && (cr1 & USART_CR1_TCIE)));
}

static int uart_stm32_irq_update(const struct device *dev) {
	return 1;
}

static void uart_stm32_irq_callback_set(const struct device *dev, 
    uart_irq_callback_user_data_t cb, void *cb_data) {
	struct stm32_uart *uart = to_uart(dev);

	uart->user_cb = cb;
	uart->user_data = cb_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->async_cb = NULL;
	data->async_user_data = NULL;
#endif
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API

static inline void async_user_callback(struct uart_stm32_data *data,
									   struct uart_event *event) {
	if (data->async_cb) {
		data->async_cb(data->uart_dev, event, data->async_user_data);
	}
}

static inline void async_evt_rx_rdy(struct uart_stm32_data *data) {
	LOG_DBG("rx_rdy: (%d %d)", data->dma_rx.offset, data->dma_rx.counter);

	struct uart_event event = {.type = UART_RX_RDY,
							   .data.rx.buf = data->dma_rx.buffer,
							   .data.rx.len = data->dma_rx.counter - data->dma_rx.offset,
							   .data.rx.offset = data->dma_rx.offset};

	/* When cyclic DMA is used, buffer positions are not updated - call callback every
	 * time*/
	if (data->dma_rx.dma_cfg.cyclic == 0) {
		/* update the current pos for new data */
		data->dma_rx.offset = data->dma_rx.counter;

		/* send event only for new data */
		if (event.data.rx.len > 0) {
			async_user_callback(data, &event);
		}
	} else {
		async_user_callback(data, &event);
	}
}

static inline void async_evt_rx_err(struct uart_stm32_data *data, int err_code) {
	LOG_DBG("rx error: %d", err_code);

	struct uart_event event = {.type = UART_RX_STOPPED,
							   .data.rx_stop.reason = err_code,
							   .data.rx_stop.data.len = data->dma_rx.counter,
							   .data.rx_stop.data.offset = 0,
							   .data.rx_stop.data.buf = data->dma_rx.buffer};

	async_user_callback(data, &event);
}

static inline void async_evt_tx_done(struct uart_stm32_data *data) {
	LOG_DBG("tx done: %d", data->dma_tx.counter);

	struct uart_event event = {.type = UART_TX_DONE,
							   .data.tx.buf = data->dma_tx.buffer,
							   .data.tx.len = data->dma_tx.counter};

	/* Reset tx buffer */
	data->dma_tx.buffer_length = 0;
	data->dma_tx.counter = 0;

	async_user_callback(data, &event);
}

static inline void async_evt_tx_abort(struct uart_stm32_data *data) {
	LOG_DBG("tx abort: %d", data->dma_tx.counter);

	struct uart_event event = {.type = UART_TX_ABORTED,
							   .data.tx.buf = data->dma_tx.buffer,
							   .data.tx.len = data->dma_tx.counter};

	/* Reset tx buffer */
	data->dma_tx.buffer_length = 0;
	data->dma_tx.counter = 0;

	async_user_callback(data, &event);
}

static inline void async_evt_rx_buf_request(struct uart_stm32_data *data) {
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};

	async_user_callback(data, &evt);
}

static inline void async_evt_rx_buf_release(struct uart_stm32_data *data) {
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->dma_rx.buffer,
	};

	async_user_callback(data, &evt);
}

static inline void async_timer_start(struct k_work_delayable *work, int32_t timeout) {
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		/* start timer */
		LOG_DBG("async timer started for %d us", timeout);
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static void uart_stm32_dma_rx_flush(const struct device *dev, int status) {
	struct dma_status stat;
	struct uart_stm32_data *data = dev->data;

	size_t rx_rcv_len = 0;

	switch (status) {
	case DMA_STATUS_COMPLETE:
		/* fully complete */
		data->dma_rx.counter = data->dma_rx.buffer_length;
		break;
	case DMA_STATUS_BLOCK:
		/* half complete */
		data->dma_rx.counter = data->dma_rx.buffer_length / 2;

		break;
	default: /* likely STM32_ASYNC_STATUS_TIMEOUT */
		if (dma_get_status(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &stat) == 0) {
			rx_rcv_len = data->dma_rx.buffer_length - stat.pending_length;
			data->dma_rx.counter = rx_rcv_len;
		}
		break;
	}

	async_evt_rx_rdy(data);

	switch (status) { /* update offset*/
	case DMA_STATUS_COMPLETE:
		/* fully complete */
		data->dma_rx.offset = 0;
		break;
	case DMA_STATUS_BLOCK:
		/* half complete */
		data->dma_rx.offset = data->dma_rx.buffer_length / 2;
		break;
	default: /* likely STM32_ASYNC_STATUS_TIMEOUT */
		data->dma_rx.offset += rx_rcv_len - data->dma_rx.offset;
		break;
	}
}

#endif /* CONFIG_UART_ASYNC_API */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API) ||           \
	defined(CONFIG_PM)

static void uart_stm32_isr(const struct device *dev) {
	struct stm32_uart *uart = to_uart(dev);
#if defined(CONFIG_PM) || defined(CONFIG_UART_ASYNC_API)
	const struct uart_stm32_config *config = dev->config;
	USART_TypeDef *usart = uart->reg;
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (uart->user_cb) {
		uart->user_cb(dev, uart->user_data);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	if (LL_USART_IsEnabledIT_IDLE(usart) && LL_USART_IsActiveFlag_IDLE(usart)) {
		LL_USART_ClearFlag_IDLE(usart);

		LOG_DBG("idle interrupt occurred");

		if (data->dma_rx.timeout == 0) {
			uart_stm32_dma_rx_flush(dev, STM32_ASYNC_STATUS_TIMEOUT);
		} else {
			/* Start the RX timer not null */
			async_timer_start(&data->dma_rx.timeout_work, data->dma_rx.timeout);
		}
	} else if (LL_USART_IsEnabledIT_TC(usart) && LL_USART_IsActiveFlag_TC(usart)) {
		LL_USART_DisableIT_TC(usart);
		/* Generate TX_DONE event when transmission is done */
		async_evt_tx_done(data);

	} else if (LL_USART_IsEnabledIT_RXNE(usart) && LL_USART_IsActiveFlag_RXNE(usart)) {
#ifdef USART_SR_RXNE
		/* clear the RXNE flag, because Rx data was not read */
		LL_USART_ClearFlag_RXNE(usart);
#else
		/* clear the RXNE by flushing the fifo, because Rx data was not read */
		LL_USART_RequestRxDataFlush(usart);
#endif /* USART_SR_RXNE */
	}

	/* Clear errors */
	uart_stm32_err_check(dev);
#endif /* CONFIG_UART_ASYNC_API */
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API || CONFIG_PM */

#ifdef CONFIG_UART_ASYNC_API

static int uart_stm32_async_callback_set(const struct device *dev,
										 uart_callback_t callback, void *user_data) {
	struct uart_stm32_data *data = dev->data;

	data->async_cb = callback;
	data->async_user_data = user_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->user_cb = NULL;
	data->user_data = NULL;
#endif

	return 0;
}

static inline void uart_stm32_dma_tx_enable(const struct device *dev) {
	const struct uart_stm32_config *config = dev->config;

	LL_USART_EnableDMAReq_TX(uart->reg);
}

static inline void uart_stm32_dma_tx_disable(const struct device *dev) {
	const struct uart_stm32_config *config = dev->config;

	LL_USART_DisableDMAReq_TX(uart->reg);
}

static inline void uart_stm32_dma_rx_enable(const struct device *dev) {
	const struct uart_stm32_config *config = dev->config;
	struct uart_stm32_data *data = dev->data;

	LL_USART_EnableDMAReq_RX(uart->reg);

	data->dma_rx.enabled = true;
}

static inline void uart_stm32_dma_rx_disable(const struct device *dev) {
	struct uart_stm32_data *data = dev->data;

	data->dma_rx.enabled = false;
}

static int uart_stm32_async_rx_disable(const struct device *dev) {
	const struct uart_stm32_config *config = dev->config;
	USART_TypeDef *usart = uart->reg;
	struct uart_stm32_data *data = dev->data;
	struct uart_event disabled_event = {.type = UART_RX_DISABLED};

	if (!data->dma_rx.enabled) {
		async_user_callback(data, &disabled_event);
		return -EFAULT;
	}

	LL_USART_DisableIT_IDLE(usart);

	uart_stm32_dma_rx_flush(dev, STM32_ASYNC_STATUS_TIMEOUT);

	async_evt_rx_buf_release(data);

	uart_stm32_dma_rx_disable(dev);

	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);

	dma_stop(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	if (data->rx_next_buffer) {
		struct uart_event rx_next_buf_release_evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = data->rx_next_buffer,
		};
		async_user_callback(data, &rx_next_buf_release_evt);
	}

	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	/* When async rx is disabled, enable interruptible instance of uart to function
	 * normally */
	LL_USART_EnableIT_RXNE(usart);

	LOG_DBG("rx: disabled");

	async_user_callback(data, &disabled_event);

	return 0;
}

void uart_stm32_dma_tx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
						  int status) {
	const struct device *uart_dev = user_data;
	struct uart_stm32_data *data = uart_dev->data;
	struct dma_status stat;
	unsigned int key = irq_lock();

	/* Disable TX */
	uart_stm32_dma_tx_disable(uart_dev);

	(void)k_work_cancel_delayable(&data->dma_tx.timeout_work);

	if (!dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = data->dma_tx.buffer_length - stat.pending_length;
	}

	data->dma_tx.buffer_length = 0;

	irq_unlock(key);
}

static void uart_stm32_dma_replace_buffer(const struct device *dev) {
	const struct uart_stm32_config *config = dev->config;
	USART_TypeDef *usart = uart->reg;
	struct uart_stm32_data *data = dev->data;

	/* Replace the buffer and reload the DMA */
	LOG_DBG("Replacing RX buffer: %d", data->rx_next_buffer_len);

	/* reload DMA */
	data->dma_rx.offset = 0;
	data->dma_rx.counter = 0;
	data->dma_rx.buffer = data->rx_next_buffer;
	data->dma_rx.buffer_length = data->rx_next_buffer_len;
	data->dma_rx.blk_cfg.block_size = data->dma_rx.buffer_length;
	data->dma_rx.blk_cfg.dest_address = (uint32_t)data->dma_rx.buffer;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	dma_reload(data->dma_rx.dma_dev, data->dma_rx.dma_channel,
			   data->dma_rx.blk_cfg.source_address, data->dma_rx.blk_cfg.dest_address,
			   data->dma_rx.blk_cfg.block_size);

	dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel);

	LL_USART_ClearFlag_IDLE(usart);

	/* Request next buffer */
	async_evt_rx_buf_request(data);
}

void uart_stm32_dma_rx_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
						  int status) {
	const struct device *uart_dev = user_data;
	struct uart_stm32_data *data = uart_dev->data;

	if (status < 0) {
		async_evt_rx_err(data, status);
		return;
	}

	(void)k_work_cancel_delayable(&data->dma_rx.timeout_work);

	/* If we are in NORMAL MODE */
	if (data->dma_rx.dma_cfg.cyclic == 0) {
		/* true since this functions occurs when buffer is full */
		data->dma_rx.counter = data->dma_rx.buffer_length;
		async_evt_rx_rdy(data);
		if (data->rx_next_buffer != NULL) {
			async_evt_rx_buf_release(data);

			/* replace the buffer when the current
			 * is full and not the same as the next
			 * one.
			 */
			uart_stm32_dma_replace_buffer(uart_dev);
		} else {
			/* Buffer full without valid next buffer,
			 * an UART_RX_DISABLED event must be generated,
			 * but uart_stm32_async_rx_disable() cannot be
			 * called in ISR context. So force the RX timeout
			 * to minimum value and let the RX timeout to do the job.
			 */
			k_work_reschedule(&data->dma_rx.timeout_work, K_TICKS(1));
		}
	} else {
		/* CIRCULAR MODE */
		uart_stm32_dma_rx_flush(data->uart_dev, status);
	}
}

static int uart_stm32_async_tx(const struct device *dev, const uint8_t *tx_data,
							   size_t buf_size, int32_t timeout) {
	const struct uart_stm32_config *config = dev->config;
	USART_TypeDef *usart = uart->reg;
	struct uart_stm32_data *data = dev->data;
	int ret;

	if (data->dma_tx.dma_dev == NULL) {
		return -ENODEV;
	}

	if (data->dma_tx.buffer_length != 0) {
		return -EBUSY;
	}

	data->dma_tx.buffer = (uint8_t *)tx_data;
	data->dma_tx.buffer_length = buf_size;
	data->dma_tx.timeout = timeout;

	LOG_DBG("tx: l=%d", data->dma_tx.buffer_length);

	/* Clear TC flag */
	LL_USART_ClearFlag_TC(usart);

	/* Enable TC interrupt so we can signal correct TX done */
	LL_USART_EnableIT_TC(usart);

	/* set source address */
	data->dma_tx.blk_cfg.source_address = (uint32_t)data->dma_tx.buffer;
	data->dma_tx.blk_cfg.block_size = data->dma_tx.buffer_length;

	ret =
		dma_config(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &data->dma_tx.dma_cfg);

	if (ret != 0) {
		LOG_ERR("dma tx config error!");
		return -EINVAL;
	}

	if (dma_start(data->dma_tx.dma_dev, data->dma_tx.dma_channel)) {
		LOG_ERR("UART err: TX DMA start failed!");
		return -EFAULT;
	}

	/* Start TX timer */
	async_timer_start(&data->dma_tx.timeout_work, data->dma_tx.timeout);

	/* Enable TX DMA requests */
	uart_stm32_dma_tx_enable(dev);

	return 0;
}

static int uart_stm32_async_rx_enable(const struct device *dev, uint8_t *rx_buf,
									  size_t buf_size, int32_t timeout) {
	const struct uart_stm32_config *config = dev->config;
	USART_TypeDef *usart = uart->reg;
	struct uart_stm32_data *data = dev->data;
	int ret;

	if (data->dma_rx.dma_dev == NULL) {
		return -ENODEV;
	}

	if (data->dma_rx.enabled) {
		LOG_WRN("RX was already enabled");
		return -EBUSY;
	}

	data->dma_rx.offset = 0;
	data->dma_rx.buffer = rx_buf;
	data->dma_rx.buffer_length = buf_size;
	data->dma_rx.counter = 0;
	data->dma_rx.timeout = timeout;

	/* Disable RX interrupts to let DMA to handle it */
	LL_USART_DisableIT_RXNE(usart);

	data->dma_rx.blk_cfg.block_size = buf_size;
	data->dma_rx.blk_cfg.dest_address = (uint32_t)data->dma_rx.buffer;

	ret =
		dma_config(data->dma_rx.dma_dev, data->dma_rx.dma_channel, &data->dma_rx.dma_cfg);

	if (ret != 0) {
		LOG_ERR("UART ERR: RX DMA config failed!");
		return -EINVAL;
	}

	if (dma_start(data->dma_rx.dma_dev, data->dma_rx.dma_channel)) {
		LOG_ERR("UART ERR: RX DMA start failed!");
		return -EFAULT;
	}

	/* Flush RX data buffer */
#ifdef USART_SR_RXNE
	LL_USART_ClearFlag_RXNE(usart);
#else
	LL_USART_RequestRxDataFlush(usart);
#endif /* USART_SR_RXNE */

	/* Enable RX DMA requests */
	uart_stm32_dma_rx_enable(dev);

	/* Enable IRQ IDLE to define the end of a
	 * RX DMA transaction.
	 */
	LL_USART_ClearFlag_IDLE(usart);
	LL_USART_EnableIT_IDLE(usart);

	LL_USART_EnableIT_ERROR(usart);

	/* Request next buffer */
	async_evt_rx_buf_request(data);

	LOG_DBG("async rx enabled");

	return ret;
}

static int uart_stm32_async_tx_abort(const struct device *dev) {
	struct uart_stm32_data *data = dev->data;
	size_t tx_buffer_length = data->dma_tx.buffer_length;
	struct dma_status stat;

	if (tx_buffer_length == 0) {
		return -EFAULT;
	}

	(void)k_work_cancel_delayable(&data->dma_tx.timeout_work);
	if (!dma_get_status(data->dma_tx.dma_dev, data->dma_tx.dma_channel, &stat)) {
		data->dma_tx.counter = tx_buffer_length - stat.pending_length;
	}

	dma_stop(data->dma_tx.dma_dev, data->dma_tx.dma_channel);
	async_evt_tx_abort(data);

	return 0;
}

static void uart_stm32_async_rx_timeout(struct k_work *work) {
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_dma_stream *rx_stream =
		CONTAINER_OF(dwork, struct uart_dma_stream, timeout_work);
	struct uart_stm32_data *data =
		CONTAINER_OF(rx_stream, struct uart_stm32_data, dma_rx);
	const struct device *dev = data->uart_dev;

	LOG_DBG("rx timeout");

	if (data->dma_rx.counter == data->dma_rx.buffer_length) {
		uart_stm32_async_rx_disable(dev);
	} else {
		uart_stm32_dma_rx_flush(dev, STM32_ASYNC_STATUS_TIMEOUT);
	}
}

static void uart_stm32_async_tx_timeout(struct k_work *work) {
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_dma_stream *tx_stream =
		CONTAINER_OF(dwork, struct uart_dma_stream, timeout_work);
	struct uart_stm32_data *data =
		CONTAINER_OF(tx_stream, struct uart_stm32_data, dma_tx);
	const struct device *dev = data->uart_dev;

	uart_stm32_async_tx_abort(dev);

	LOG_DBG("tx: async timeout");
}

static int uart_stm32_async_rx_buf_rsp(const struct device *dev, uint8_t *buf,
									   size_t len) {
	struct uart_stm32_data *data = dev->data;
	unsigned int key;
	int err = 0;

	LOG_DBG("replace buffer (%d)", len);

	key = irq_lock();

	if (data->rx_next_buffer != NULL) {
		err = -EBUSY;
	} else if (!data->dma_rx.enabled) {
		err = -EACCES;
	} else {
		data->rx_next_buffer = buf;
		data->rx_next_buffer_len = len;
	}

	irq_unlock(key);

	return err;
}

static int uart_stm32_async_init(const struct device *dev) {
	const struct uart_stm32_config *config = dev->config;
	USART_TypeDef *usart = uart->reg;
	struct uart_stm32_data *data = dev->data;

	data->uart_dev = dev;

	if (data->dma_rx.dma_dev != NULL) {
		if (!device_is_ready(data->dma_rx.dma_dev)) {
			return -ENODEV;
		}
	}

	if (data->dma_tx.dma_dev != NULL) {
		if (!device_is_ready(data->dma_tx.dma_dev)) {
			return -ENODEV;
		}
	}

	/* Disable both TX and RX DMA requests */
	uart_stm32_dma_rx_disable(dev);
	uart_stm32_dma_tx_disable(dev);

	k_work_init_delayable(&data->dma_rx.timeout_work, uart_stm32_async_rx_timeout);
	k_work_init_delayable(&data->dma_tx.timeout_work, uart_stm32_async_tx_timeout);

	/* Configure dma rx config */
	memset(&data->dma_rx.blk_cfg, 0, sizeof(data->dma_rx.blk_cfg));

#if defined(CONFIG_SOC_SERIES_STM32F1X) || defined(CONFIG_SOC_SERIES_STM32F2X) ||        \
	defined(CONFIG_SOC_SERIES_STM32F4X) || defined(CONFIG_SOC_SERIES_STM32L1X)
	data->dma_rx.blk_cfg.source_address = LL_USART_DMA_GetRegAddr(usart);
#else
	data->dma_rx.blk_cfg.source_address =
		LL_USART_DMA_GetRegAddr(usart, LL_USART_DMA_REG_DATA_RECEIVE);
#endif

	data->dma_rx.blk_cfg.dest_address = 0; /* dest not ready */

	if (data->dma_rx.src_addr_increment) {
		data->dma_rx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_rx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	if (data->dma_rx.dst_addr_increment) {
		data->dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_rx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	/* Enable/disable RX circular buffer */
	data->dma_rx.blk_cfg.source_reload_en = data->dma_rx.dma_cfg.cyclic;
	data->dma_rx.blk_cfg.dest_reload_en = data->dma_rx.dma_cfg.cyclic;

	data->dma_rx.blk_cfg.fifo_mode_control = data->dma_rx.fifo_threshold;

	data->dma_rx.dma_cfg.head_block = &data->dma_rx.blk_cfg;
	data->dma_rx.dma_cfg.user_data = (void *)dev;
	data->rx_next_buffer = NULL;
	data->rx_next_buffer_len = 0;

	/* Configure dma tx config */
	memset(&data->dma_tx.blk_cfg, 0, sizeof(data->dma_tx.blk_cfg));

#if defined(CONFIG_SOC_SERIES_STM32F1X) || defined(CONFIG_SOC_SERIES_STM32F2X) ||        \
	defined(CONFIG_SOC_SERIES_STM32F4X) || defined(CONFIG_SOC_SERIES_STM32L1X)
	data->dma_tx.blk_cfg.dest_address = LL_USART_DMA_GetRegAddr(usart);
#else
	data->dma_tx.blk_cfg.dest_address =
		LL_USART_DMA_GetRegAddr(usart, LL_USART_DMA_REG_DATA_TRANSMIT);
#endif

	data->dma_tx.blk_cfg.source_address = 0; /* not ready */

	if (data->dma_tx.src_addr_increment) {
		data->dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_tx.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	if (data->dma_tx.dst_addr_increment) {
		data->dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_tx.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	/* Enable/disable TX circular buffer */
	data->dma_tx.blk_cfg.source_reload_en = data->dma_tx.dma_cfg.cyclic;
	data->dma_tx.blk_cfg.dest_reload_en = data->dma_tx.dma_cfg.cyclic;

	data->dma_tx.blk_cfg.fifo_mode_control = data->dma_tx.fifo_threshold;

	data->dma_tx.dma_cfg.head_block = &data->dma_tx.blk_cfg;
	data->dma_tx.dma_cfg.user_data = (void *)dev;

	return 0;
}

#endif /* CONFIG_UART_ASYNC_API */

static struct stm32_uart stm32_uart1 = {
	.class = {
		.name = "uart1",
		.poll_in = uart_stm32_poll_in,
		.poll_out = uart_stm32_poll_out,
		.err_check = uart_stm32_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
		.configure = uart_stm32_configure,
		.config_get = uart_stm32_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
		.fifo_fill = uart_stm32_fifo_fill,
		.fifo_read = uart_stm32_fifo_read,
		.irq_tx_enable = uart_stm32_irq_tx_enable,
		.irq_tx_disable = uart_stm32_irq_tx_disable,
		.irq_tx_ready = uart_stm32_irq_tx_ready,
		.irq_tx_complete = uart_stm32_irq_tx_complete,
		.irq_rx_enable = uart_stm32_irq_rx_enable,
		.irq_rx_disable = uart_stm32_irq_rx_disable,
		.irq_rx_ready = uart_stm32_irq_rx_ready,
		.irq_err_enable = uart_stm32_irq_err_enable,
		.irq_err_disable = uart_stm32_irq_err_disable,
		.irq_is_pending = uart_stm32_irq_is_pending,
		.irq_update = uart_stm32_irq_update,
		.irq_callback_set = uart_stm32_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
		.callback_set = uart_stm32_async_callback_set,
		.tx = uart_stm32_async_tx,
		.tx_abort = uart_stm32_async_tx_abort,
		.rx_enable = uart_stm32_async_rx_enable,
		.rx_disable = uart_stm32_async_rx_disable,
		.rx_buf_rsp = uart_stm32_async_rx_buf_rsp,
#endif /* CONFIG_UART_ASYNC_API */
	},
    .reg     = USART1,
    .txchan  = {.id = LL_DMAMUX1_REQ_USART1_TX, .en = 0},
    .rxchan  = {.id = LL_DMAMUX1_REQ_USART1_RX, .en = 1},
    .irq     = USART1_IRQn,
    .clksrc  = LL_RCC_USART16_CLKSOURCE,
    .clkbit  = LL_APB2_GRP1_PERIPH_USART1,
};

static int uart_stm32_registers_configure(struct stm32_uart *uart) {
	USART_TypeDef *usart = uart->reg;
	struct uart_config *uart_cfg = &uart->param;
    int err;

    // LL_USART_DeInit(uart->reg);
	LL_USART_Disable(usart);

    err = request_irq(uart->irq, (void *)uart_stm32_isr, uart);
    if (err)
        return err;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    err = uart_stm32_clkset(uart, true);
    if (err)
        return err;
#endif

	/* TX/RX direction */
	LL_USART_SetTransferDirection(usart, LL_USART_DIRECTION_TX_RX);

	/* Set basic parameters, such as data-/stop-bit, parity, and baudrate */
	uart_stm32_parameters_set((struct device *)uart, uart_cfg);

	/* Enable the single wire / half-duplex mode */
	if (uart->single_wire) {
		LL_USART_EnableHalfDuplex(usart);
	}

#if HAS_DRIVER_ENABLE
	if (uart->de_enable) {
		if (!IS_UART_DRIVER_ENABLE_INSTANCE(usart)) {
			LOG_ERR("%s does not support driver enable", dev->name);
			return -EINVAL;
		}

		uart_stm32_set_driver_enable(dev, true);
		LL_USART_SetDEAssertionTime(usart, config->de_assert_time);
		LL_USART_SetDEDeassertionTime(usart, config->de_deassert_time);

		if (uart->de_invert) {
			LL_USART_SetDESignalPolarity(usart, LL_USART_DE_POLARITY_LOW);
		}
	}
#endif

#ifdef USART_CR1_FIFOEN
	LL_USART_EnableFIFO(usart);
#endif

	LL_USART_Enable(usart);

#ifdef USART_ISR_TEACK
	/* Wait until TEACK flag is set */
	while (!(LL_USART_IsActiveFlag_TEACK(usart))) {
	}
#endif /* !USART_ISR_TEACK */

#ifdef USART_ISR_REACK
	/* Wait until REACK flag is set */
	while (!(LL_USART_IsActiveFlag_REACK(usart))) {
	}
#endif /* !USART_ISR_REACK */

	return device_register((struct device *)&uart->class);
}

static struct device *uart_console;
static struct stm32_uart stm32_console;

static void stm32_uart_puts(const char *s, size_t len) {
   USART_TypeDef *reg = to_uart(uart_console)->reg;
	while (len > 0) {
		while (!(reg->ISR & LL_USART_ISR_TXE_TXFNF));
		reg->TDR = (uint8_t)*s++;
		len--;
	}
}

static int stm32_uart_init(void) {
    int err;

    err = uart_stm32_registers_configure(&stm32_uart1);
    if (!err) {
        const struct uart_config cfg = {
            .baudrate = 2000000,
            .data_bits = UART_CFG_DATA_BITS_8,
            .parity = UART_CFG_PARITY_NONE,
            .stop_bits = UART_CFG_STOP_BITS_1,
            .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
        };

        uart_console = device_find("uart1");
        err = uart_configure(uart_console, &cfg);
        if (!err) {
            __console_puts = stm32_uart_puts;

			/* Register console device */
			memcpy(&stm32_console, uart_console, sizeof(struct stm32_uart));
			stm32_console.class.name = "console";
			device_register((struct device *)&stm32_console.class);
		}
    }

    return err;
}

SYSINIT(stm32_uart_init, SI_DRIVER_LEVEL, 00);