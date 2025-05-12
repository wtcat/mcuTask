/*
 * Copyright 2024 wtcat
 */
#ifndef BASE_DEBUG_KASAN_H_
#define BASE_DEBUG_KASAN_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KASAN_BYTES_PER_WORD (sizeof(uintptr_t))
#define KASAN_BITS_PER_WORD (KASAN_BYTES_PER_WORD * 8)
#define KASAN_FIRST_WORD_MASK(start)                                                     \
	(UINTPTR_MAX << ((start) & (KASAN_BITS_PER_WORD - 1)))
#define KASAN_LAST_WORD_MASK(end) (UINTPTR_MAX >> (-(end) & (KASAN_BITS_PER_WORD - 1)))
#define KASAN_SHADOW_SCALE (sizeof(uintptr_t))
#define KASAN_SHADOW_SIZE(size)                                                          \
	(KASAN_BYTES_PER_WORD * ((size) / KASAN_SHADOW_SCALE / KASAN_BITS_PER_WORD))
#define KASAN_REGION_STRUCT_SIZE (sizeof(uintptr_t) * 4)

#ifdef CONFIG_KASAN
#define KASAN_REGION_SIZE(size) (KASAN_REGION_STRUCT_SIZE + KASAN_SHADOW_SIZE(size))
#else
#define KASAN_REGION_SIZE(size) (0)
#endif /* CONFIG_KASAN */

#ifdef CONFIG_KASAN
void kasan_poison(const void *addr, size_t size);
void kasan_unpoison(const void *addr, size_t size);
void kasan_register(void *addr, size_t *size);
#else /* !CONFIG_KASAN */
#define kasan_poison(...)
#define kasan_unpoison(...)
#define kasan_register(...)
#endif /* CONFIG_KASAN */

#ifdef __cplusplus
}
#endif
#endif /* BASE_DEBUG_KASAN_H_ */
