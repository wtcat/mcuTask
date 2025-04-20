/*
 * Copyright 2024 wtcat
 */
#define TX_USE_BOARD_PRIVATE

#include <errno.h>
#include <tx_api.h>
#include <service/irq.h>
#include <service/printk.h>


struct irq_desc {
	void (*handler)(void *arg);
	void *arg;
};

static struct irq_desc _irqdesc_table[BOARD_IRQ_MAX] __fastbss;

static void default_irq_handler(void *arg) {
	printk("Warnning***: please install interrupt(%d) handler\n", 
		(int)IRQ_VECTOR_GET());
}

void __fastcode dispatch_irq(void) {
	int irq = IRQ_VECTOR_GET();
	struct irq_desc *desc = _irqdesc_table + irq;

#ifdef TX_EXECUTION_PROFILE_ENABLE
    _tx_execution_isr_enter();
#endif
	desc->handler(desc->arg);
#ifdef TX_EXECUTION_PROFILE_ENABLE
    _tx_execution_isr_exit();
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

	enable_irq(irq);
	return 0;
}

int remove_irq(int irq, void (*handler)(void *), void *arg) {
	if (irq >= BOARD_IRQ_MAX)
		return -EINVAL;

	(void) handler;
	(void) arg;
	disable_irq(irq);

	scoped_guard(os_irq) {
		_irqdesc_table[irq].handler = default_irq_handler;
		_irqdesc_table[irq].arg = NULL;
	}
	return 0;
}

int init_irq(void) {
	/* Setup default interrupt handler */
	for (size_t i = 0; i < rte_array_size(_irqdesc_table); i++) {
		_irqdesc_table[i].handler = default_irq_handler;
		_irqdesc_table[i].arg = NULL;
	}

    return 0;
}
