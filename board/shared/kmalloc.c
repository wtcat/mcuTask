/*
 * Copyright 2024 wtcat
 */

#include <tx_api.h>
#include <service/init.h>
#include <service/malloc.h>

extern char _kernel_byte_pool_start[];
#ifndef CONFIG_SIMULATOR
extern char _kernel_byte_pool_size[];
#else
extern UINT _kernel_byte_pool_size;
#endif /* CONFIG_SIMULATOR */

static TX_BYTE_POOL kernel_byte_pool __fastdata;


void * __kmalloc(size_t size, unsigned int flags) {
    void *ptr = NULL;
    tx_byte_allocate(&kernel_byte_pool,
        &ptr, size, (flags & GMF_WAIT)? TX_WAIT_FOREVER: TX_NO_WAIT);
    return ptr;
}

void *__kzalloc(size_t size, unsigned int flags) {
    void *ptr = __kmalloc(size, flags);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

void __kfree(void *ptr) {
    tx_byte_release(ptr);
}


static int kmalloc_init(void) {
    tx_byte_pool_create(&kernel_byte_pool, "kernel",
        _kernel_byte_pool_start, (ULONG)_kernel_byte_pool_size);
    return 0;
}

SYSINIT(kmalloc_init, SI_MEMORY_LEVEL, 10);
