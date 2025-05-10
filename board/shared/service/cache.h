/*
 * Copyright (c) 2025 Carlo wtcat
 */


#ifndef SERVICE_CACHE_H_
#define SERVICE_CACHE_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C"{
#endif

#ifdef CONFIG_DCACHE
#define cache_data_enable    arch_dcache_enable
#define cache_data_disable   arch_dcache_disable
#define cache_data_flush_all arch_dcache_flush_all
#define cache_data_invd_all  arch_dcache_invd_all
#define cache_data_flush_and_invd_all arch_dcache_flush_and_invd_all
#define cache_data_flush_range(addr, size) arch_dcache_flush_range(addr, size)
#define cache_data_invd_range(addr, size) arch_dcache_invd_range(addr, size)
#define cache_data_flush_and_invd_range(addr, size) \
	arch_dcache_flush_and_invd_range(addr, size)
#define cache_data_line_size_get arch_dcache_line_size_get
#else /* !CONFIG_DCACHE */

#define cache_data_enable(...)
#define cache_data_disable(...)
#define cache_data_flush_all(...)
#define cache_data_invd_all(...)
#define cache_data_flush_and_invd_all(...)
#define cache_data_flush_range(...)
#define cache_data_invd_range(...)
#define cache_data_flush_and_invd_range(...)
#define cache_data_line_size_get(...)
#endif /* CONFIG_DCACHE */

#ifdef CONFIG_ICACHE
#define cache_instr_enable arch_icache_enable
#define cache_instr_disable arch_icache_disable
#define cache_instr_flush_all arch_icache_flush_all
#define cache_instr_invd_all arch_icache_invd_all
#define cache_instr_flush_and_invd_all arch_icache_flush_and_invd_all
#define cache_instr_flush_range(addr, size) arch_icache_flush_range(addr, size)
#define cache_instr_invd_range(addr, size) arch_icache_invd_range(addr, size)
#define cache_instr_flush_and_invd_range(addr, size) \
	arch_icache_flush_and_invd_range(addr, size)
#define cache_instr_line_size_get arch_icache_line_size_get
#else /* !CONFIG_ICACHE */

#define cache_instr_enable(...)
#define cache_instr_disable(...)
#define cache_instr_flush_all(...)
#define cache_instr_invd_all(...)
#define cache_instr_flush_and_invd_all(...)
#define cache_instr_flush_range(...)
#define cache_instr_invd_range(...)
#define cache_instr_flush_and_invd_range(...)
#define cache_instr_line_size_get(...)
#endif /* CONFIG_ICACHE */


#ifdef CONFIG_DCACHE
/**
 * @brief Enable the d-cache
 *
 * Enable the data cache.
 */
void arch_dcache_enable(void);

/**
 * @brief Disable the d-cache
 *
 * Disable the data cache.
 */
void arch_dcache_disable(void);

/**
 * @brief Flush the d-cache
 *
 * Flush the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_all(void);

/**
 * @brief Invalidate the d-cache
 *
 * Invalidate the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_invd_all(void);

/**
 * @brief Flush and Invalidate the d-cache
 *
 * Flush and Invalidate the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_and_invd_all(void);

/**
 * @brief Flush an address range in the d-cache
 *
 * Flush the specified address range of the data cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being flushed, all the portions of the
 *       data structures sharing the same line will be flushed. This is usually
 *       not a problem because writing back is a non-destructive process that
 *       could be triggered by hardware at any time, so having an aligned
 *       @p addr or a padded @p size is not strictly necessary.
 *
 * @param addr Starting address to flush.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_range(void *addr, size_t size);

/**
 * @brief Invalidate an address range in the d-cache
 *
 * Invalidate the specified address range of the data cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being invalidated, all the portions of the
 *       non-read-only data structures sharing the same line will be
 *       invalidated as well. This is a destructive process that could lead to
 *       data loss and/or corruption. When @p addr is not aligned to the cache
 *       line and/or @p size is not a multiple of the cache line size the
 *       behaviour is undefined.
 *
 * @param addr Starting address to invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_invd_range(void *addr, size_t size);

/**
 * @brief Flush and Invalidate an address range in the d-cache
 *
 * Flush and Invalidate the specified address range of the data cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being flushed, all the portions of the
 *       data structures sharing the same line will be flushed before being
 *       invalidated. This is usually not a problem because writing back is a
 *       non-destructive process that could be triggered by hardware at any
 *       time, so having an aligned @p addr or a padded @p size is not strictly
 *       necessary.
 *
 * @param addr Starting address to flush and invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */

int arch_dcache_flush_and_invd_range(void *addr, size_t size);

#ifdef CONFIG_DCACHE_LINE_SIZE_DETECT

/**
 *
 * @brief Get the d-cache line size.
 *
 * The API is provided to dynamically detect the data cache line size at run
 * time.
 *
 * The function must be implemented only when CONFIG_DCACHE_LINE_SIZE_DETECT is
 * defined.
 *
 * @retval size Size of the d-cache line.
 * @retval 0 If the d-cache is not enabled.
 */
size_t arch_dcache_line_size_get(void);

#endif /* CONFIG_DCACHE_LINE_SIZE_DETECT */
#endif /* CONFIG_DCACHE */

#ifdef CONFIG_ICACHE

/**
 * @brief Enable the i-cache
 *
 * Enable the instruction cache.
 */
void arch_icache_enable(void);

/**
 * @brief Disable the i-cache
 *
 * Disable the instruction cache.
 */
void arch_icache_disable(void);

/**
 * @brief Flush the i-cache
 *
 * Flush the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_all(void);

/**
 * @brief Invalidate the i-cache
 *
 * Invalidate the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_invd_all(void);

/**
 * @brief Flush and Invalidate the i-cache
 *
 * Flush and Invalidate the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_and_invd_all(void);

/**
 * @brief Flush an address range in the i-cache
 *
 * Flush the specified address range of the instruction cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being flushed, all the portions of the
 *       data structures sharing the same line will be flushed. This is usually
 *       not a problem because writing back is a non-destructive process that
 *       could be triggered by hardware at any time, so having an aligned
 *       @p addr or a padded @p size is not strictly necessary.
 *
 * @param addr Starting address to flush.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_range(void *addr, size_t size);

/**
 * @brief Invalidate an address range in the i-cache
 *
 * Invalidate the specified address range of the instruction cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being invalidated, all the portions of the
 *       non-read-only data structures sharing the same line will be
 *       invalidated as well. This is a destructive process that could lead to
 *       data loss and/or corruption. When @p addr is not aligned to the cache
 *       line and/or @p size is not a multiple of the cache line size the
 *       behaviour is undefined.
 *
 * @param addr Starting address to invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_invd_range(void *addr, size_t size);

/**
 * @brief Flush and Invalidate an address range in the i-cache
 *
 * Flush and Invalidate the specified address range of the instruction cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being flushed, all the portions of the
 *       data structures sharing the same line will be flushed before being
 *       invalidated. This is usually not a problem because writing back is a
 *       non-destructive process that could be triggered by hardware at any
 *       time, so having an aligned @p addr or a padded @p size is not strictly
 *       necessary.
 *
 * @param addr Starting address to flush and invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_and_invd_range(void *addr, size_t size);

#ifdef CONFIG_ICACHE_LINE_SIZE_DETECT
/**
 *
 * @brief Get the i-cache line size.
 *
 * The API is provided to dynamically detect the instruction cache line size at
 * run time.
 *
 * The function must be implemented only when CONFIG_ICACHE_LINE_SIZE_DETECT is
 * defined.
 *
 * @retval size Size of the d-cache line.
 * @retval 0 If the d-cache is not enabled.
 */

size_t arch_icache_line_size_get(void);

#endif /* CONFIG_ICACHE_LINE_SIZE_DETECT */
#endif /* CONFIG_ICACHE */

void arch_cache_init(void);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif
#endif /* SERVICE_CACHE_H_ */
