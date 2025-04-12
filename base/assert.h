/*
 * Copyright 2023 wtcat
 */

#ifndef BASE_ASSERT_H_
#define BASE_ASSERT_H_

#ifdef CONFIG_HEADER_FILE
#include CONFIG_HEADER_FILE
#endif

#ifdef __cplusplus
extern "C"{
#endif

/**
 * assert() equivalent, that is always enabled.
 */
#define __rte_assert0(cond, msg) do {                                   \
    if (!(cond)) {                                                      \
        __assert_failed(__FILE__, __LINE__, __func__, msg);             \
    }                                                                   \
} while (0)

/**
 * assert() equivalent, that does not lie in speed critical code.
 * These asserts() thus can be enabled without fearing speed loss.
 */

#define rte_assert0(cond) __rte_assert0(cond, #cond)

#ifndef CONFIG_ASSERT_DISABLE
#define rte_assert(cond) rte_assert0(cond)
#define rte_assert_msg(cond, _msg) __rte_assert0(cond, _msg)
#else
#define rte_assert(...)
#define rte_assert_msg(...)
#endif

#define RTE_ASSERT(_e) rte_assert(_e)

void __assert_failed(const char *file, int line, const char *func, 
    const char *failedexpr);

#ifdef __cplusplus
}
#endif
#endif /* BASE_ASSERT_H_ */
