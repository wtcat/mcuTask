/*
 * Copyright 2024 wtcat
 */
#ifndef BASE__RTE_CPU_ARM_H_
#define BASE__RTE_CPU_ARM_H_

#ifdef CONFIG_HEADER_FILE
#include CONFIG_HEADER_FILE
#endif

#ifndef RTE_CACHE_LINE_SIZE
#ifdef CONFIG_ARM64
#define RTE_CACHE_LINE_SIZE 64
#else
#define RTE_CACHE_LINE_SIZE 32
#endif
#endif /* RTE_CACHE_LINE_SIZE */

#endif /* BASE__RTE_CPU_ARM_H_ */
