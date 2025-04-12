/*
 * Copyright 2024 wtcat
 */

#ifndef TX_USER_H_
#define TX_USER_H_

#include "rtos/configs/tx_user_config.h"

/* Profile thread execution state */
#define TX_EXECUTION_TIME_SOURCE   (EXECUTION_TIME_SOURCE_TYPE) HRTIMER_CYCLE_TO_US(HRTIMER_JIFFIES)


#define TX_MSEC(n) (n * 1000 / TX_TIMER_TICKS_PER_SECOND)
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
#define __fastcode  __rte_section(".itcm")
#define __fastbss   __rte_section(".fastbss")
#define __fastdata  __rte_section(".fastdata")

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

/* GPIO */
extern GPIO_TypeDef *stm32_gpio_ports[];

#define STM32_GPIO(_p, _n, _pull)     (((_pull) << 16) | (((_p) - 'A') << 8) | (_n))
#define STM32_GPIO_PIN(_gpio)  ((_gpio) & 0xFF)
#define STM32_GPIO_PORT(_gpio) (((_gpio) >> 8) & 0xFF)
#define STM32_GPIO_PULL(_gpio) (((_gpio) >> 16) & 0xFF)

#define STM32_PINS_SET(_port, _mask) \
   stm32_gpio_ports[(_port)]->BSRR |= (_mask)

#define STM32_PINS_CLR(_port, _mask) \
   stm32_gpio_ports[(_port)]->BSRR |= ((_mask) << 16)


static inline void stm32_pin_set(uint32_t gpio, int value) {
   int shift = STM32_GPIO_PIN(gpio) + (!value << 4);
   STM32_PINS_SET(STM32_GPIO_PORT(gpio), 1 << shift);
}

static inline int stm32_pin_get(uint32_t gpio) {
   uint32_t inp = stm32_gpio_ports[STM32_GPIO_PORT(gpio)]->IDR;
   int pin = STM32_GPIO_PIN(gpio);
   return !!(inp & (1 << pin));
}

void cortexm_systick_handler(void);

#endif /* TX_USE_BOARD_PRIVATE */

#endif /* TX_USER_H_ */
