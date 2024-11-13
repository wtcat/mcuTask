/*
 * Copyright 2024 wtcat
 */
#include "tx_api.h"
#include "basework/os/osapi.h"

LINKER_ROSET(sysinit, struct sysinit_item);

static void do_sysinit(void) {
    LINKER_SET_FOREACH(sysinit, item, struct sysinit_item) {
        int err = item->handler();
        if (err)
            printk("Failed to execute %s (%d)\n", item->name, err);
    }
}

static void demo_test(void);
void tx_application_define(void *unused) {
    do_sysinit();
    demo_test();
}

int main(void) {
    tx_kernel_enter();
    return 0;
}


static void demo_thread_1(ULONG arg) {
    int count = 0;

    for ( ; ; ) {
        printk("Thread-1: count(%d)\n", count++);
        tx_thread_sleep(TX_MSEC(1000));
    }
}

static void demo_thread_2(ULONG arg) {
    int count = 0;

    for ( ; ; ) {
        printk("Thread-2: count(%d)\n", count++);
        tx_thread_sleep(TX_MSEC(1000));
    }
}

static void demo_test(void) {
    static TX_THREAD tx_demo1;
    static ULONG txdemo_stack1[1024/sizeof(ULONG)] __rte_section(".dtcm");

    static TX_THREAD tx_demo2;
    static ULONG txdemo_stack2[1024/sizeof(ULONG)] __rte_section(".dtcm");

    _tx_thread_create(&tx_demo1, "demo", demo_thread_1, 0, txdemo_stack1, 
        sizeof(txdemo_stack1), 10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);
    _tx_thread_create(&tx_demo2, "demo", demo_thread_2, 0, txdemo_stack2, 
        sizeof(txdemo_stack2), 11, 11, TX_NO_TIME_SLICE, TX_AUTO_START);
}
