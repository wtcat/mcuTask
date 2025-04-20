/*
 * Copyright 2024 wtcat
 */

#include <service/init.h>
#include <service/printk.h>

LINKER_ROSET(sysinit, struct sysinit_item);

void do_sysinit(void) {
    int no = 0;

    printk("\n*** System Initialize ***\n");
    LINKER_SET_FOREACH(sysinit, item, struct sysinit_item) {
        printk("[%02d] => %s\n", no++, item->name);
        int err = item->handler();
        if (err)
            printk("!Failed to execute %s (%d)\n", item->name, err);
    }
}
