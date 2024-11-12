/*
 * Copyright 2024 wtcat
 */
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>

#include "tx_api.h"
#include "tx_thread.h"
#include "basework/lib/iovpr.h"

#include "stm32h7xx.h"

struct irq_desc {
	void (*handler)(void *arg);
	void *arg;
};

#define LINKER_SYM(_sym) extern char _sym[];
#define VECTOR_SIZE (WAKEUP_PIN_IRQn + 17)
#define SYSTEM_CLK 1000000

LINKER_SYM(_sbss)
LINKER_SYM(_ebss)

LINKER_SYM(_sdata)
LINKER_SYM(_eronly)
LINKER_SYM(_edata)

LINKER_SYM(_sramfuncs)
LINKER_SYM(_eramfuncs)
LINKER_SYM(_framfuncs)

LINKER_SYM(_fsbss)
LINKER_SYM(_febss)

LINKER_SYM(_fsdata)
LINKER_SYM(_fedata)
LINKER_SYM(_fdataload)

extern int main(void);
extern void _tx_timer_interrupt(void);
extern void __tx_PendSVHandler(void);
static void stm32_exception_handler(void);
static void stm32_systick_handler(void);
static void stm32_irq_dispatch(void);
void _stm32_reset(void);

static struct irq_desc _irqdesc_table[WAKEUP_PIN_IRQn + 1] __fastbss;
static char _main_stack[4096] __fastbss __rte_aligned(8);
static void *_ram_vectors[VECTOR_SIZE] __rte_section(".ram_vectors");
static const void *const irq_vectors[VECTOR_SIZE] __rte_section(".vectors") __rte_used = {

	/* Initial stack */
	_main_stack + sizeof(_main_stack),

	/* Reset exception handler */
	(void *)_stm32_reset,

	(void *)stm32_exception_handler,  /* NMI */
	(void *)stm32_exception_handler,  /* Hard Fault */
	(void *)stm32_exception_handler,  /* MPU Fault */
	(void *)stm32_exception_handler,  /* Bus Fault */
	(void *)stm32_exception_handler,  /* Usage Fault */
	(void *)stm32_exception_handler,  /* Reserved */
	(void *)stm32_exception_handler,  /* Reserved */
	(void *)stm32_exception_handler,  /* Reserved */
	(void *)stm32_exception_handler, /* Reserved */
	(void *)stm32_exception_handler, /* SVC */
	(void *)stm32_exception_handler, /* Debug Monitor */
	(void *)stm32_exception_handler, /* Reserved */
	(void *)__tx_PendSVHandler, /* PendSV */
	(void *)stm32_systick_handler, /* SysTick */

	[16 ... WAKEUP_PIN_IRQn+16] = (void *)stm32_irq_dispatch
};

static void __rte_naked stm32_exception_handler(void) {
	while (1);
}

static void default_irq_handler(void *arg) {
	(void) arg;
	while (1);
}

void _stm32_reset(void) {
	const uint32_t *src;
	uint32_t *dest;

	/* Clear bss section */
	for (dest = (uint32_t *)_sbss; dest < (uint32_t *)_ebss;)
		*dest++ = 0;

	/* Initialize data section */
	for (src = (const uint32_t *)_eronly, dest = (uint32_t *)_sdata;
		 dest < (uint32_t *)_edata;)
		*dest++ = *src++;

	/* Initialize ramfunc */
	for (src = (const uint32_t *)_framfuncs, dest = (uint32_t *)_sramfuncs;
		 dest < (uint32_t *)_eramfuncs;)
		*dest++ = *src++;

	/* Initialize dtcm */
	for (dest = (uint32_t *)_fsbss; dest < (uint32_t *)_febss;)
		*dest++ = 0;
	for (src = (const uint32_t *)_fdataload, dest = (uint32_t *)_fsdata;
		 dest < (uint32_t *)_fedata;)
		*dest++ = *src++;

	printk("stm32 starting ...\n");

	/* Redirect vector table to ram region*/
	memcpy(_ram_vectors, irq_vectors, VECTOR_SIZE);
	SCB->VTOR = (uint32_t)_ram_vectors;
	__DSB();

	/* Initialize HAL layer */
	HAL_Init();

	/* Clock initialize */

	/* Enable systick */
	SysTick->CTRL = 0;
    HAL_SYSTICK_Config((HAL_RCCEx_GetD1SysClockFreq()) / TX_TIMER_TICKS_PER_SECOND);
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	/* Configure the UART so that we can get debug output as soon as possible */
	// stm32_clockconfig();	
	// arm_fpuconfig();
	// stm32_lowsetup();

	// /* Enable/disable tightly coupled memories */
	// stm32_tcmenable();

	// /* Initialize onboard resources */
	// stm32_boardinitialize();

	/* Enable I- and D-Caches */
	SCB_EnableICache();
	SCB_EnableDCache();

	main();
}

static void __fastcode stm32_systick_handler(void) {
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    _tx_execution_isr_enter()
#endif
	_tx_timer_interrupt();
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    _tx_execution_isr_exit()
#endif
}

static void __fastcode stm32_irq_dispatch(void) {
	int vector = SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;
	struct irq_desc *desc = _irqdesc_table + vector;

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    _tx_execution_isr_enter()
#endif
	desc->handler(desc->arg);
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    _tx_execution_isr_exit()
#endif
}

void _tx_initialize_low_level(void) {
	_tx_thread_system_stack_ptr = (void *)irq_vectors[0];
}

int request_irq(int irq, void (*handler)(void *), void *arg) {
	TX_INTERRUPT_SAVE_AREA

	if (irq >= WAKEUP_PIN_IRQn)
		return -EINVAL;
	if (!handler)
		return -EINVAL;

	
	TX_DISABLE
	_irqdesc_table[irq].handler = handler;
	_irqdesc_table[irq].arg = arg;
	TX_RESTORE

	NVIC_EnableIRQ(irq);
	return 0;
}

int remove_irq(int irq, void (*handler)(void *), void *arg) {
	TX_INTERRUPT_SAVE_AREA

	if (irq >= WAKEUP_PIN_IRQn)
		return -EINVAL;

	(void) handler;
	(void) arg;
	NVIC_DisableIRQ(irq);

	TX_DISABLE
	_irqdesc_table[irq].handler = default_irq_handler;
	_irqdesc_table[irq].arg = NULL;
	TX_RESTORE

	return 0;
}

static void __fastcode put_char(int c, void *arg) {
	stm32_uart_putc((char)c);
	if (c == '\n')
		stm32_uart_putc('\r');
}

int printk(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	int len = _IO_Vprintf(put_char, NULL, fmt, ap);
	va_end(ap);

	return len;
}
