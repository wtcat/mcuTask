/*
 * Copyright(c) 2015 RehiveTech. All rights reserved.
 * Copyright 2024 wtcat
 */

#ifndef BASE__RTE_PREFETCH_ARM_H_
#define BASE__RTE_PREFETCH_ARM_H_

#include "base/generic.h"
#include "base/arch/generic/rte_prefetch.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void rte_prefetch0(const volatile void *p) {
	asm volatile ("pld [%0]" : : "r" (p));
}

static inline void rte_prefetch1(const volatile void *p) {
	asm volatile ("pld [%0]" : : "r" (p));
}

static inline void rte_prefetch2(const volatile void *p) {
	asm volatile ("pld [%0]" : : "r" (p));
}

static inline void rte_prefetch_non_temporal(const volatile void *p) {
	/* non-temporal version not available, fallback to rte_prefetch0 */
	rte_prefetch0(p);
}

#ifdef __cplusplus
}
#endif

#endif /* BASE__RTE_PREFETCH_ARM_H_ */
