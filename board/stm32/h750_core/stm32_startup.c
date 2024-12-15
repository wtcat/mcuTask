/*
 * Copyright 2024 wtcat
 */
#define TX_USE_BOARD_PRIVATE
#define TX_USE_SECTION_INIT_API_EXTENSION 1
#include <stdint.h>

#include "tx_api.h"
#include "tx_thread.h"

#include "stm32h7xx_hal.h"


#define VECTOR_SIZE  (VECTOR_MAX + 16)
#define VECTOR_MAX   (WAKEUP_PIN_IRQn + 1)

extern int main(void);
extern void PendSV_Handler(void);

void _stm32_exception_handler(void);
void _stm32_reset(void);
static void stm32_clock_setup(void);
static void early_console_init(void);


static char _main_stack[4096] __rte_section(".dtcm") __rte_aligned(8);
static void *_ram_vectors[VECTOR_SIZE] __rte_section(".ram_vectors");
static const void *const irq_vectors[VECTOR_SIZE] __rte_section(".vectors") __rte_used = {

	/* Initial stack */
	_main_stack + sizeof(_main_stack),

	/* Reset exception handler */
	(void *)_stm32_reset,

	(void *)_stm32_exception_handler,  /* NMI */
	(void *)_stm32_exception_handler,  /* Hard Fault */
	(void *)_stm32_exception_handler,  /* MPU Fault */
	(void *)_stm32_exception_handler,  /* Bus Fault */
	(void *)_stm32_exception_handler,  /* Usage Fault */
	(void *)_stm32_exception_handler,  /* Reserved */
	(void *)_stm32_exception_handler,  /* Reserved */
	(void *)_stm32_exception_handler,  /* Reserved */
	(void *)_stm32_exception_handler, /* Reserved */
	(void *)_stm32_exception_handler, /* SVC */
	(void *)_stm32_exception_handler, /* Debug Monitor */
	(void *)_stm32_exception_handler, /* Reserved */
	(void *)PendSV_Handler, /* PendSV */
	(void *)cortexm_systick_handler, /* SysTick */

	[16 ... VECTOR_SIZE-1] = (void *)dispatch_irq
};

static void switch_to_threadsp(void) {
	__asm__ volatile(
		"mrs r2, control\n"
		"orr r2, #0x2\n"
		"msr control, r2\n"
		"isb\n"
		"dsb\n"
	);
}
/*
 * Avoid xxxx_section to be optimized to memset/memcpy
 */
void __attribute__((optimize("O0"))) 
_stm32_reset(void) {
	SystemInit();

	__set_PSP((uint32_t)&_main_stack[2048]);

	/* Clear bss section */
	_clear_bss_section(_sbss, _ebss);

	/* Initialize data section */
	_copy_data_section(_sdata, _edata, _eronly);

	/* Initialize ramfunc */
	_copy_data_section(_sramfuncs, _eramfuncs, _framfuncs);

	/* Initialize dtcm */
	_clear_bss_section(_fsbss, _febss);
	_copy_data_section(_fsdata, _fedata, _fdataload);

	/* Redirect vector table to ram region*/
	memcpy(_ram_vectors, irq_vectors, sizeof(_ram_vectors));
	SCB->VTOR = (uint32_t)_ram_vectors;
	__DSB();

	/* Clock initialize */
	stm32_clock_setup();
	early_console_init();

	/* Enable I- and D-Caches */
	SCB_EnableICache();
	SCB_EnableDCache();
	
	printk("stm32 starting ...\n");
	_tx_thread_system_stack_ptr = (void *)irq_vectors[0];

	switch_to_threadsp();
	/* Schedule kernel */
	tx_kernel_enter();

	/* 
	 * Should never reached here 
	 */
	while (1);
}

static void stm32_clock_setup(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

	__HAL_RCC_SYSCFG_CLK_ENABLE();

	/** Supply configuration update enable
	*/
	HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
	/** Configure the main internal regulator output voltage
	*/
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

	while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

	/** Macro to configure the PLL clock source
	*/
	__HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48 
									| RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_DIV4;
	RCC_OscInitStruct.HSICalibrationValue = 64;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 5;
	RCC_OscInitStruct.PLL.PLLN = 192;
	RCC_OscInitStruct.PLL.PLLP = 2;
	RCC_OscInitStruct.PLL.PLLQ = 2;
	RCC_OscInitStruct.PLL.PLLR = 2;
	RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
	RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
	RCC_OscInitStruct.PLL.PLLFRACN = 0;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		while (1);

	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
								|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
								|RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
	RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
		while (1);
	
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC | RCC_PERIPHCLK_USART3
											| RCC_PERIPHCLK_UART4
											| RCC_PERIPHCLK_SPI123 | RCC_PERIPHCLK_SPI45
											| RCC_PERIPHCLK_I2C123 | RCC_PERIPHCLK_I2C4
											| RCC_PERIPHCLK_SDMMC
											| RCC_PERIPHCLK_USB|RCC_PERIPHCLK_FMC;
	PeriphClkInitStruct.PLL2.PLL2M = 2;
	PeriphClkInitStruct.PLL2.PLL2N = 64;
	PeriphClkInitStruct.PLL2.PLL2P = 4;
	PeriphClkInitStruct.PLL2.PLL2Q = 2;
	PeriphClkInitStruct.PLL2.PLL2R = 4;
	PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
	PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
	PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
	PeriphClkInitStruct.PLL3.PLL3M = 5;
	PeriphClkInitStruct.PLL3.PLL3N = 160;
	PeriphClkInitStruct.PLL3.PLL3P = 8;
	PeriphClkInitStruct.PLL3.PLL3Q = 8;
	PeriphClkInitStruct.PLL3.PLL3R = 8;
	PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_2;
	PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
	PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
	PeriphClkInitStruct.FmcClockSelection = RCC_FMCCLKSOURCE_PLL2;
	PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL2;
	PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL2;
	PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_PLL3;
	PeriphClkInitStruct.I2c123ClockSelection = RCC_I2C123CLKSOURCE_HSI;
	PeriphClkInitStruct.I2c4ClockSelection = RCC_I2C4CLKSOURCE_HSI;
	PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
	PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
		while (1);
	
	/** Enable USB Voltage detector
	*/
	HAL_PWREx_EnableUSBVoltageDetector();
}

/*
 * Console port
 */
#define CONSOLE_PORT (USART1)
#define CONSOLE_SPEED 2000000

static void early_puts(const char *s, size_t len) {
	while (len > 0) {
		while (!(CONSOLE_PORT->ISR & LL_USART_ISR_TXE_TXFNF));
		CONSOLE_PORT->TDR = (uint8_t)*s++;
		len--;
	}
}

static int early_getc(void) {
    if (CONSOLE_PORT->ISR & LL_USART_ISR_RXNE_RXFNE)
        return CONSOLE_PORT->RDR & 0xFF;
    return -1;
}

static void early_console_init(void) {
	uint32_t clkfreq;
	uint32_t cr1;

    /*
     * PA9  -> USART1_TX
     * PA10 -> USART1_RX
     */
	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	clkfreq = LL_RCC_GetUSARTClockFreq(LL_RCC_USART16_CLKSOURCE);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
	cr1 = CONSOLE_PORT->CR1;
	cr1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE | 
            USART_CR1_RE | USART_CR1_OVER8 | USART_CR1_UE);
	
	cr1 |= LL_USART_PARITY_NONE | LL_USART_DATAWIDTH_8B;
	CONSOLE_PORT->CR1 = cr1;
	LL_USART_SetPrescaler(CONSOLE_PORT, 0);
	LL_USART_SetBaudRate(CONSOLE_PORT, clkfreq, 0, 0, CONSOLE_SPEED);
	LL_USART_SetStopBitsLength(CONSOLE_PORT, LL_USART_STOPBITS_1);
	LL_USART_SetHWFlowCtrl(CONSOLE_PORT, 0);
	LL_USART_EnableDirectionTx(CONSOLE_PORT);
	LL_USART_EnableDirectionRx(CONSOLE_PORT);
	CONSOLE_PORT->CR1 |= USART_CR1_UE;

	__console_puts = early_puts;
	__console_getc = early_getc;
}
