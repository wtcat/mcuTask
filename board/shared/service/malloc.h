/*
 * Copyright 2025 wtcat
 */
#ifndef SERVICE_MALLOC_H_
#define SERVICE_MALLOC_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C"{
#endif

/*
 * Memory allocate interface
 */
#define GMF_KERNEL 0x0001
#define GMF_WAIT   0x0002

void *__kmalloc(size_t size, unsigned int gmf);
void *__kzalloc(size_t size, unsigned int gmf);
void  __kfree(void *ptr);

#define kmalloc(s, f) __kmalloc(s, f)
#define kzalloc(s, f) __kzalloc(s, f)
#define kfree(p)      __kfree(p)


#ifdef __cplusplus
}
#endif
#endif /* SERVICE_MALLOC_H_ */
