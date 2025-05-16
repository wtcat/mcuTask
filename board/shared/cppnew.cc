/*
 * Copyright (c) 2018
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <new>

#include <base/assert.h>
#include <service/malloc.h>
#include <service/heap.h>

#if __cplusplus < 201103L
#define NOEXCEPT
#else /* >= C++11 */
#define NOEXCEPT noexcept
#endif /* __cplusplus */

#if __cplusplus < 202002L
#define NODISCARD
#else
#define NODISCARD [[nodiscard]]
#endif /* __cplusplus */

#if CONFIG_CPP_POOL_SIZE > 0
static BYTE_HEAP_DEFINE(cpp_pool, CONFIG_CPP_POOL_SIZE, );

#define CC_MALLOC(_size)               __kasan_heap_allocate(&cpp_pool, size, TX_NO_WAIT)
#define CC_ALIGN_ALLOC(_align, _size)  __kasan_heap_allocate(&cpp_pool, size, TX_NO_WAIT)
#define CC_FREE(_ptr)                  __kasan_heap_free(_ptr)

#else
#define CC_MALLOC(_size)                kmalloc(_size, 0)
#define CC_ALIGN_ALLOC(_align, _size)   kmalloc(_size, 0)
#define CC_FREE(_ptr)                   kfree(_ptr)
#endif /* CONFIG_CPP_HEAP_SIZE > 0 */

NODISCARD void* operator new(size_t size)
{
	return CC_MALLOC(size);
}

NODISCARD void* operator new[](size_t size)
{
	return CC_MALLOC(size);
}

NODISCARD void* operator new(std::size_t size, const std::nothrow_t& tag) NOEXCEPT
{
	return CC_MALLOC(size);
}

NODISCARD void* operator new[](std::size_t size, const std::nothrow_t& tag) NOEXCEPT
{
	return CC_MALLOC(size);
}

#if __cplusplus >= 201703L
NODISCARD void* operator new(size_t size, std::align_val_t al)
{
	rte_assert((size_t)al <= sizeof(void *));
	return CC_ALIGN_ALLOC(static_cast<size_t>(al), size);
}

NODISCARD void* operator new[](std::size_t size, std::align_val_t al)
{
	rte_assert((size_t)al <= sizeof(void *));
	return CC_ALIGN_ALLOC(static_cast<size_t>(al), size);
}

NODISCARD void* operator new(std::size_t size, std::align_val_t al,
			     const std::nothrow_t&) NOEXCEPT
{
	rte_assert((size_t)al <= sizeof(void *));
	return CC_ALIGN_ALLOC(static_cast<size_t>(al), size);
}

NODISCARD void* operator new[](std::size_t size, std::align_val_t al,
			       const std::nothrow_t&) NOEXCEPT
{
	rte_assert((size_t)al <= sizeof(void *));
	return CC_ALIGN_ALLOC(static_cast<size_t>(al), size);
}
#endif /* __cplusplus >= 201703L */

void operator delete(void* ptr) NOEXCEPT
{
	CC_FREE(ptr);
}

void operator delete[](void* ptr) NOEXCEPT
{
	CC_FREE(ptr);
}

#if (__cplusplus > 201103L)
void operator delete(void* ptr, size_t) NOEXCEPT
{
	CC_FREE(ptr);
}

void operator delete[](void* ptr, size_t) NOEXCEPT
{
	CC_FREE(ptr);
}
#endif // __cplusplus > 201103L
