/*
 * Copyright 2024 wtcat
 */

#ifndef TX_USER_H_
#define TX_USER_H_

#include "rtos/configs/tx_user_config.h"

/* Profile thread execution state */
#define TX_EXECUTION_TIME_SOURCE   (EXECUTION_TIME_SOURCE_TYPE) HRTIMER_CYCLE_TO_US(HRTIMER_JIFFIES)


#if TX_TIMER_TICKS_PER_SECOND == 1000
#define TX_MSEC(n) (n)
#define TX_USEC(n) ((n) / 1000)

#else
#define TX_MSEC(n) (((n) * TX_TIMER_TICKS_PER_SECOND) / 1000)
#define TX_USEC(n) (((n) * TX_TIMER_TICKS_PER_SECOND) / 1000000)
#endif /* TX_TIMER_TICKS_PER_SECOND == 1000 */

#define TX_DISABLE_ERROR_CHECKING

/*
 * CPU architecture configuration
 */
#define TX_ENABLE_WFI  /* Support wfi instruction */
// #define TX_PORT_USE_BASEPRI 
// #define TX_PORT_BASEPRI 0x80


/*
 * Enable tx-api extension
 */
#define TX_THREAD_API_EXTENSION



/*
 * User general extension configuration
 */
#define TX_SYSTEM_PANIC() for ( ; ; )

/* Device driver */
#define TX_UART_DEVICE_STUB  /* Optimized uart driver performance */

/* Task runner */
#define TX_TASK_RUNNER_STACK_SIZE 1024
#define TX_TASK_RUNNER_PRIO 12

/* */
#define __fastcode    __rte_section(".itcm")
#define __fastbss     __rte_section(".fastbss")
#define __fastdata    __rte_section(".fastdata")

/* HR-Timer */
#define HR_TIMER_PRESCALER 5
#define HRTIMER_US(n) ((n) * (240 / HR_TIMER_PRESCALER))
#define HRTIMER_JIFFIES  *((volatile uint32_t *)0x40000024UL)
#define HRTIMER_CYCLE_TO_US(n) ((n) / (240 / HR_TIMER_PRESCALER))

/*
 * FileX for filesystem
 */
#define CONFIG_FILEX_MEDIA_BUFFER_SIZE 4096
#define CONFIG_FILEX_MAX_FILES 3


/*
 * Board private
 */

#ifdef TX_USE_BOARD_PRIVATE
#include "stm32h7xx.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_usart.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_crc.h"
#include "stm32h7xx_ll_tim.h"
#include "stm32h7xx_ll_i2c.h"
#include "stm32h7xx_ll_spi.h"
#include "stm32h7xx_ll_sdmmc.h"

/* Systick */
#define BOARD_IRQ_MAX 150
#define BOARD_SYSTICK_CLKFREQ HAL_RCCEx_GetD1SysClockFreq()

#define IRQ_VECTOR_GET()  ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) - 16)

/* Console */
#define CONSOLE_DEFAULT_SPEED 2000000

/* GPIO speed */
#define STM32_OSPEEDR_LOW_SPEED		(0x0 << STM32_OSPEEDR_SHIFT)
#define STM32_OSPEEDR_MEDIUM_SPEED	(0x1 << STM32_OSPEEDR_SHIFT)
#define STM32_OSPEEDR_HIGH_SPEED	(0x2 << STM32_OSPEEDR_SHIFT)
#define STM32_OSPEEDR_VERY_HIGH_SPEED	(0x3 << STM32_OSPEEDR_SHIFT)
#define STM32_OSPEEDR_MASK		0x3
#define STM32_OSPEEDR_SHIFT		7

/* GPIO */
#define STM32_AF0     0x0
#define STM32_AF1     0x1
#define STM32_AF2     0x2
#define STM32_AF3     0x3
#define STM32_AF4     0x4
#define STM32_AF5     0x5
#define STM32_AF6     0x6
#define STM32_AF7     0x7
#define STM32_AF8     0x8
#define STM32_AF9     0x9
#define STM32_AF10    0xa
#define STM32_AF11    0xb
#define STM32_AF12    0xc
#define STM32_AF13    0xd
#define STM32_AF14    0xe
#define STM32_AF15    0xf
#define STM32_ANALOG  0x10
#define STM32_GPIO    0x11

/**
 * @brief Macro to generate pinmux int using port, pin number and mode arguments
 * This is inspired from Linux equivalent st,stm32f429-pinctrl binding
 */

#define STM32_MODE_SHIFT 0U
#define STM32_MODE_MASK  0x1FU
#define STM32_LINE_SHIFT 5U
#define STM32_LINE_MASK  0xFU
#define STM32_PORT_SHIFT 9U
#define STM32_PORT_MASK  0x1FU

/**
 * @brief Pin configuration configuration bit field.
 *
 * Fields:
 *
 * - mode [ 0 : 4 ]
 * - line [ 5 : 8 ]
 * - port [ 9 : 13 ]
 *
 * @param port Port ('A'..'Q')
 * @param line Pin (0..15)
 * @param mode Mode (ANALOG, GPIO_IN, ALTERNATE).
 */
#define STM32_PINMUX(port, line, mode)					       \
		(((((port) - 'A') & STM32_PORT_MASK) << STM32_PORT_SHIFT) |    \
		(((line) & STM32_LINE_MASK) << STM32_LINE_SHIFT) |	       \
		(((STM32_ ## mode) & STM32_MODE_MASK) << STM32_MODE_SHIFT))

#define STM32_PINMUX_PORT(mux) (((mux) >> STM32_PORT_SHIFT) & STM32_PORT_MASK)
#define STM32_PINMUX_PIN(mux)  (((mux) >> STM32_LINE_SHIFT) & STM32_LINE_MASK)
#define STM32_PINMUX_ALT(mux)  (((mux) >> STM32_MODE_SHIFT) & STM32_MODE_MASK)

void cortexm_systick_handler(void);

#endif /* TX_USE_BOARD_PRIVATE */

#endif /* TX_USER_H_ */
