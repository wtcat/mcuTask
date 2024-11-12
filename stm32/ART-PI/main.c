/*
 * Copyright 2024 wtcat
 */
#include "tx_api.h"

LINKER_ROSET(sysinit, struct sysinit_item);

static void do_sysinit(void) {
    LINKER_SET_FOREACH(sysinit, item, struct sysinit_item) {
        int err = item->handler();
        if (err)
            printk("Failed to execute %s (%d)\n", item->name, err);
    }
}

void tx_application_define(void *unused) {
    do_sysinit();
}

int main(void) {
    tx_kernel_enter();
    return 0;
}
