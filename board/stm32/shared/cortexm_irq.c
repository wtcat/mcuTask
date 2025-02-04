/*
 * Copyright 2024 wtcat
 */
#define TX_USE_BOARD_PRIVATE

#include "tx_api.h"

extern void _tx_timer_interrupt(void);

void __fastcode cortexm_systick_handler(void) {
#ifdef TX_EXECUTION_PROFILE_ENABLE
    _tx_execution_isr_enter();
#endif

	_tx_timer_interrupt();
	
#ifdef TX_EXECUTION_PROFILE_ENABLE
    _tx_execution_isr_exit();
#endif
}

void _tx_initialize_low_level(void) {
	NVIC_SetPriorityGrouping(__NVIC_PRIO_BITS);
	for (int i = 0; i < BOARD_IRQ_MAX; i++) {
		NVIC_DisableIRQ(i);
		NVIC_ClearPendingIRQ(i);
		NVIC_SetPriority(i, 10);
	}
	NVIC_SetPriority(SVCall_IRQn, 15);
	NVIC_SetPriority(PendSV_IRQn, 15);
	NVIC_SetPriority(SysTick_IRQn, 13);

	init_irq();

	/* Enable systick */
	uint32_t ticks = BOARD_SYSTICK_CLKFREQ / TX_TIMER_TICKS_PER_SECOND;
	SysTick->CTRL = 0;
	SysTick->LOAD = ticks - 1;
	SysTick->VAL  = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk
				  | SysTick_CTRL_ENABLE_Msk;
}

int enable_irq(int irq) {
	NVIC_EnableIRQ(irq);
	return 0;
}

int disable_irq(int irq) {
	NVIC_DisableIRQ(irq);
	return 0;
}
