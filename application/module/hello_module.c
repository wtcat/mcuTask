/*
 * Copyright 2025 wtcat
 */

#include <service/printk.h>
#include <base/symbol.h>

void hello_world(void) {
    printk("Hello World\n");
}

EXPORT_SYMBOL(hello_world);
