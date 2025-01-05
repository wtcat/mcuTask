/*
 * Copyright 2024 wtcat
 */

#include "tx_api.h"


void *object_allocate(struct object_pool *pool) {
    void *obj;
    scoped_guard(os_irq) {
        obj = pool->free_chain;
        if (obj) 
            pool->free_chain = *(char **)obj;
    }
    return obj;
}

void object_free(struct object_pool *pool, void *obj) {
    if (obj == NULL)
        return;

    scoped_guard(os_irq) {
        *(char **)obj = pool->free_chain;
        pool->free_chain = obj;
    }
}

int object_pool_initialize(struct object_pool *pool, void *buffer, size_t size, 
    size_t objsize) {
    size_t n, fixed_isize = rte_roundup(objsize, sizeof(void *));
    char *p, *head = NULL;

    scoped_guard(os_irq) {
        for (p = buffer, n = size / fixed_isize; n > 0; n--) {
            *(char **)p = head;
            head = p;
            p = p + fixed_isize;
        }
        pool->free_chain = head;
    }

    return 0;
}