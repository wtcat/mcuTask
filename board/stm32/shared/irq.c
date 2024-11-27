/*
 * Copyright 2024 wtcat
 */
#define TX_USE_BOARD_PRIVATE

#include <errno.h>
#include "tx_api.h"

struct irq_desc {
	void (*handler)(void *arg);
	void *arg;
};

#define VECTOR_GET() (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk)
#define IRQ_GET()    (VECTOR_GET() - 16)

extern void _tx_timer_interrupt(void);
static struct irq_desc _irqdesc_table[BOARD_IRQ_MAX] __fastbss;

static void default_irq_handler(void *arg) {
	printk("Warnning***: please install interrupt(%d) handler\n", 
		(int)IRQ_GET());
}

void __fastcode platform_systick_handler(void) {
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    _tx_execution_isr_enter()
#endif
	_tx_timer_interrupt();
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    _tx_execution_isr_exit()
#endif
}

void _tx_initialize_low_level(void) {
	for (int i = 0; i < BOARD_IRQ_MAX; i++) {
		NVIC_DisableIRQ(i);
		NVIC_ClearPendingIRQ(i);
		NVIC_SetPriority(i, 10);
	}
	NVIC_SetPriority(SVCall_IRQn, 15);
	NVIC_SetPriority(PendSV_IRQn, 15);
	NVIC_SetPriority(SysTick_IRQn, 13);

	/* Setup default interrupt handler */
	for (size_t i = 0; i < rte_array_size(_irqdesc_table); i++) {
		_irqdesc_table[i].handler = default_irq_handler;
		_irqdesc_table[i].arg = NULL;
	}

	/* Enable systick */
	uint32_t ticks = BOARD_SYSTICK_CLKFREQ / TX_TIMER_TICKS_PER_SECOND;
	SysTick->CTRL = 0;
	SysTick->LOAD = ticks - 1;
	SysTick->VAL  = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk
				  | SysTick_CTRL_ENABLE_Msk;
}

void __fastcode platform_irq_dispatch(void) {
	int irq = IRQ_GET();
	struct irq_desc *desc = _irqdesc_table + irq;

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    _tx_execution_isr_enter()
#endif
	desc->handler(desc->arg);
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    _tx_execution_isr_exit()
#endif
}

int request_irq(int irq, void (*handler)(void *), void *arg) {
	if (irq >= BOARD_IRQ_MAX)
		return -EINVAL;

	if (!handler)
		return -EINVAL;

	scoped_guard(os_irq) {
		_irqdesc_table[irq].handler = handler;
		_irqdesc_table[irq].arg = arg;
	}

	NVIC_EnableIRQ(irq);
	return 0;
}

int remove_irq(int irq, void (*handler)(void *), void *arg) {
	if (irq >= BOARD_IRQ_MAX)
		return -EINVAL;

	(void) handler;
	(void) arg;
	NVIC_DisableIRQ(irq);

	scoped_guard(os_irq) {
		_irqdesc_table[irq].handler = default_irq_handler;
		_irqdesc_table[irq].arg = NULL;
	}
	return 0;
}
