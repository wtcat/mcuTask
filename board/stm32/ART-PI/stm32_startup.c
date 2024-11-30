/*
 * Copyright 2024 wtcat
 */
#define TX_USE_BOARD_PRIVATE
#include <stdint.h>

#include "tx_api.h"
#include "tx_thread.h"


#define VECTOR_SIZE  (VECTOR_MAX + 16)
#define VECTOR_MAX   (WAKEUP_PIN_IRQn + 1)

extern int main(void);
extern void PendSV_Handler(void);

void _stm32_exception_handler(void);
void _stm32_reset(void);


static char _main_stack[4096] __fastbss __rte_aligned(8);
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

/*
 * Avoid xxxx_section to be optimized to memset/memcpy
 */
void __attribute__((optimize("O0"))) 
_stm32_reset(void) {
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

	/* Initialize HAL layer */
	HAL_Init();

	/* Clock initialize */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

	/* Enable I- and D-Caches */
	SCB_EnableICache();
	SCB_EnableDCache();

	printk("stm32 starting ...\n");
	_tx_thread_system_stack_ptr = (void *)irq_vectors[0];
	main();

	/* 
	 * Should never reached here 
	 */
	while (1);
}
