/*
 * Copyright 2024 wtcat
 */

#include "tx_api.h"

extern char _app_pool_byte_start[];
extern char _app_pool_byte_size[];

static TX_BYTE_POOL app_byte_pool __fastdata; 

void *__general_malloc(size_t size) {
    void *p = NULL;
    tx_byte_allocate(&app_byte_pool, &p, size, TX_NO_WAIT);
    return p;
}

void *__general_calloc(size_t n, size_t size) {
    void *p = NULL;
    tx_byte_allocate(&app_byte_pool, &p, n *size, TX_NO_WAIT);
    return p;
}

void __general_free(void *ptr) {
    tx_byte_release(ptr);
}

void *__dma_coherent_alloc(size_t size, unsigned int flags) {
    return NULL;
}

void __dma_coherent_free(void *ptr) {
    
}

static int malloc_init(void) {
    tx_byte_pool_create(&app_byte_pool, "application",
        _app_pool_byte_start, (ULONG)_app_pool_byte_size);
    return 0;
}

SYSINIT(malloc_init, SI_MEMORY_LEVEL, 10);
