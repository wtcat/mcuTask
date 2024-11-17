/*
 * Copyright 2024 wtcat
 */
#include "tx_api.h"
#include "basework/os/osapi.h"

extern char __kernel_pool_start[];
extern char __kernel_pool_size[];
extern char __app_pool_start[];
extern char __app_pool_size[];

static TX_BYTE_POOL kernel_pool;
static TX_BYTE_POOL app_pool;

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
    tx_byte_pool_create(&kernel_pool, "kernel",
        __kernel_pool_start, (ULONG)__kernel_pool_size);
    tx_byte_pool_create(&app_pool, "application",
        __app_pool_start, (ULONG)__app_pool_size);

    do_sysinit();
    demo_test();
}

int main(void) {
    tx_kernel_enter();
    return 0;
}

/*
 * Memory allocate implemention
 */
void *__general_malloc(size_t size) {
    void *p = NULL;
    tx_byte_allocate(&kernel_pool, &p, size, TX_NO_WAIT);
    return p;
}

void *__general_calloc(size_t n, size_t size) {
    void *p = NULL;
    tx_byte_allocate(&kernel_pool, &p, n *size, TX_NO_WAIT);
    return p;
}

void __general_free(void *ptr) {
    tx_byte_release(ptr);
}

void *kmalloc(size_t size, unsigned int flags) {
    void *ptr = NULL;
    tx_byte_allocate((flags & GMF_KERNEL)? &kernel_pool: &app_pool, 
        &ptr, size, (flags & GMF_WAIT)? TX_WAIT_FOREVER: TX_NO_WAIT);
    return ptr;
}

void *kzalloc(size_t size, unsigned int flags) {
    void *ptr = kmalloc(size, flags);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

void kfree(void *ptr) {
    tx_byte_release(ptr);
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

    tx_thread_create(&tx_demo1, "demo", demo_thread_1, 0, txdemo_stack1, 
        sizeof(txdemo_stack1), 10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&tx_demo2, "demo", demo_thread_2, 0, txdemo_stack2, 
        sizeof(txdemo_stack2), 11, 11, TX_NO_TIME_SLICE, TX_AUTO_START);
}
