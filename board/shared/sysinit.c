/*
 * Copyright 2024 wtcat
 */
#include "tx_api.h"

LINKER_ROSET(sysinit, struct sysinit_item);

void do_sysinit(void) {
    LINKER_SET_FOREACH(sysinit, item, struct sysinit_item) {
        int err = item->handler();
        if (err)
            printk("Failed to execute %s (%d)\n", item->name, err);
    }
}
