/*
 * Copyright 2025 wtcat
 */
#ifndef BASE_BARRIER_H_
#define BASE_BARRIER_H_

#ifdef CONFIG_HEADER_FILE
#include CONFIG_HEADER_FILE
#endif

#include <stdatomic.h>

#ifdef __cplusplus
extern "C"{
#endif

#ifndef rte_mb
#define	rte_mb()  __atomic_thread_fence(__ATOMIC_SEQ_CST)
#endif

#ifndef rte_wmb
#define	rte_wmb() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#endif

#ifndef rte_rmb
#define	rte_rmb() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#endif

#ifdef __cplusplus
}
#endif
#endif /* BASE_BARRIER_H_ */
