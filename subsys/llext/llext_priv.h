/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LLEXT_PRIV_H_
#define ZEPHYR_SUBSYS_LLEXT_PRIV_H_

#include <tx_api.h>
#include <service/heap.h>
#include <subsys/llext/llext.h>
#include <subsys/llext/llext_internal.h>

/*
 * Memory management (llext_mem.c)
 */

int llext_copy_strings(struct llext_loader *ldr, struct llext *ext);
int llext_copy_regions(struct llext_loader *ldr, struct llext *ext,
		       const struct llext_load_param *ldr_parm);
void llext_free_regions(struct llext *ext);
void llext_adjust_mmu_permissions(struct llext *ext);

static inline void *llext_alloc(size_t bytes)
{
	extern TX_BYTE_POOL llext_heap;
	return __kasan_heap_allocate(&llext_heap, bytes, TX_NO_WAIT);
}

static inline void *llext_aligned_alloc(size_t align, size_t bytes)
{
	(void) align;
	return llext_alloc(bytes);
}

static inline void llext_free(void *ptr)
{
	__kasan_heap_free(ptr);
}

/*
 * ELF parsing (llext_load.c)
 */

int do_llext_load(struct llext_loader *ldr, struct llext *ext,
		  const struct llext_load_param *ldr_parm);

static inline const char *llext_string(const struct llext_loader *ldr, const struct llext *ext,
				       enum llext_mem mem_idx, unsigned int idx)
{
	return (char *)ext->mem[mem_idx] + idx;
}

/*
 * Relocation (llext_link.c)
 */

int llext_link(struct llext_loader *ldr, struct llext *ext,
	       const struct llext_load_param *ldr_parm);
ssize_t llext_file_offset(struct llext_loader *ldr, uintptr_t offset);
void llext_dependency_remove_all(struct llext *ext);

#endif /* ZEPHYR_SUBSYS_LLEXT_PRIV_H_ */
