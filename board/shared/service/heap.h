/*
 * Copyright 2025 wtcat
 */
#ifndef SERVICE_HEAP_H_
#define SERVICE_HEAP_H_

#include <service/init.h>
#include <base/trace/kasan.h>

#ifdef __cplusplus
extern "C"{
#endif

struct TX_BYTE_POOL_STRUCT;
typedef struct TX_BYTE_POOL_STRUCT TX_BYTE_POOL;

/*
 * Byte heap define
 *
 * @name: heap name
 * @size: heap size
 * @attr: heap attribute
 */
#define BYTE_HEAP_DEFINE(name, size, attr) \
    TX_BYTE_POOL name attr; \
    static char name##_heap_buffer[size + KASAN_REGION_SIZE(size)] \
        __rte_aligned(sizeof(void *)) attr; \
    static int name##_heap_init(void) { \
        return __kasan_heap_init(&name, #name, name##_heap_buffer, \
            sizeof(name##_heap_buffer)); \
    } \
    SYSINIT(name##_heap_init, SI_MEMORY_LEVEL, 10)

/*
 * __kasan_heap_init - Initialize byte heap
 *
 * @pool: pointer to heap object
 * @name: pointer to heap name
 * @pool_start: pointer to heap buffer
 * @pool_size:  heap size
 * return 0 if success
 */
int __kasan_heap_init(TX_BYTE_POOL *pool, const char* name, void *pool_start, 
    size_t pool_size);

/*
 * __kasan_heap_allocate - Allocate buffer from heap  
 *
 * @pool: pointer to heap object
 * @size: the size of request buffer
 * @options: allocate options
 * return no-null address if success
 */
void *__kasan_heap_allocate(TX_BYTE_POOL *pool, size_t size, unsigned long options);

/*
 * __kasan_heap_free - Free buffer to heap  
 *
 * @ptr: pointer to buffer address
 */
void __kasan_heap_free(void *ptr);

#ifdef __cplusplus
}
#endif
#endif /* SERVICE_HEAP_H_ */
