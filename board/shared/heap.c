/*
 * Copyright 2025 wtcat
 */

#include <tx_api.h>
#include <tx_byte_pool.h>
#include <service/heap.h>

#ifdef CONFIG_KASAN
#include <base/trace/kasan.h>
#endif

struct block_header {
    UCHAR *next;
    union {
        ALIGN_TYPE state;
        TX_BYTE_POOL *pool;
    };
};

int __rte_no_kasan
__kasan_heap_init(TX_BYTE_POOL *pool, const char* name, void *pool_start, 
    size_t pool_size) {
    int err;
#ifdef CONFIG_KASAN
    kasan_register(pool_start, &pool_size);
#endif
    err = tx_byte_pool_create(pool, (CHAR *)name, pool_start, pool_size);
    return -err;
}

void *__rte_no_kasan 
__kasan_heap_allocate(TX_BYTE_POOL *pool, size_t size, unsigned long options) {
    void *ptr;
    if (tx_byte_allocate(pool, &ptr, size, options) == TX_SUCCESS) {
#ifdef CONFIG_KASAN
        kasan_unpoison(ptr, rte_roundup(size, sizeof(uintptr_t)));
#endif
        return ptr;
    }
    return NULL;
}

void __rte_no_kasan 
__kasan_heap_free(void *ptr) {
#ifdef CONFIG_KASAN
    if (rte_likely(ptr != NULL)) {
        struct block_header *p = ptr - sizeof(struct block_header);
        if (p->pool->tx_byte_pool_id == TX_BYTE_POOL_ID)
            kasan_poison(p, (size_t)TX_UCHAR_POINTER_DIF(p->next, p));
    }
#endif /* CONFIG_KASAN */
    tx_byte_release(ptr);
}
