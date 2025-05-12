/*
 * Copyright 2024 wtcat
 */

#include <tx_api.h>
#include <service/malloc.h>
#include <service/heap.h>


static BYTE_HEAP_DEFINE(kmalloc_pool, CONFIG_KMALLOC_POOL_SIZE, );

void * __kmalloc(size_t size, unsigned int flags) {
    return __kasan_heap_allocate(&kmalloc_pool, size, 
        (flags & GMF_WAIT)? TX_WAIT_FOREVER: TX_NO_WAIT);
}

void *__kzalloc(size_t size, unsigned int flags) {
    void *ptr = __kmalloc(size, flags);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

void __kfree(void *ptr) {
    __kasan_heap_free(ptr);
}
