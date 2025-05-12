/*
 * Copyright 2024 wtcat
 */

#include <tx_api.h>
#include <service/heap.h>
#include <service/malloc.h>

#if CONFIG_GMALLOC_POOL_SIZE > 0
static BYTE_HEAP_DEFINE(gmalloc_pool, CONFIG_GMALLOC_POOL_SIZE, );
#endif

void *__general_malloc(size_t size) {
#if CONFIG_GMALLOC_POOL_SIZE > 0
    return __kasan_heap_allocate(&gmalloc_pool, size, TX_NO_WAIT);
#else
    return __kmalloc(size, 0);
#endif /* CONFIG_GMALLOC_POOL_SIZE > 0 */
}

void *__general_calloc(size_t n, size_t size) {
#if CONFIG_GMALLOC_POOL_SIZE > 0
    void *ptr = __kasan_heap_allocate(&gmalloc_pool, n * size, TX_NO_WAIT);
    if (ptr)
        memset(ptr, 0, n * size);
    return ptr;
#else
    return __kzalloc(n * size, 0);
#endif /* CONFIG_GMALLOC_POOL_SIZE > 0 */
}

void __general_free(void *ptr) {
#if CONFIG_GMALLOC_POOL_SIZE > 0
    __kasan_heap_free(ptr);
#else
    __kfree(ptr);
#endif /* CONFIG_GMALLOC_POOL_SIZE > 0 */
}
