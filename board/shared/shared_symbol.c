/*
 * Copyright 2025 wtcat
 */

#include <service/irq.h>
#include <service/malloc.h>
#include <service/printk.h>

#include <base/symbol.h>

EXPORT_SYMBOL(request_irq);
EXPORT_SYMBOL(remove_irq);
EXPORT_SYMBOL(enable_irq);
EXPORT_SYMBOL(disable_irq);

EXPORT_SYMBOL(__kmalloc);
EXPORT_SYMBOL(__kzalloc);
EXPORT_SYMBOL(__kfree);

EXPORT_SYMBOL(printk);
