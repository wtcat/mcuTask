/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define pr_fmt(fmt) "[llext_handlers]: "fmt

#include <errno.h>
#include <string.h>

#include <subsys/llext/llext.h>
#include <subsys/llext/loader.h>
#include <subsys/llext/llext_priv.h>
#include <base/log.h>


ssize_t llext_get_fn_table(struct llext *ext, bool is_init, void *buf,
								  size_t buf_size) {
	size_t table_size;

	if (!ext) {
		return -EINVAL;
	}

	if (is_init) {
		table_size = ext->mem_size[LLEXT_MEM_PREINIT] + ext->mem_size[LLEXT_MEM_INIT];
	} else {
		table_size = ext->mem_size[LLEXT_MEM_FINI];
	}

	if (buf) {
		char *byte_ptr = buf;

		if (buf_size < table_size) {
			return -ENOMEM;
		}

		if (is_init) {
			/* setup functions from preinit_array and init_array */
			memcpy(byte_ptr, ext->mem[LLEXT_MEM_PREINIT],
				   ext->mem_size[LLEXT_MEM_PREINIT]);
			memcpy(byte_ptr + ext->mem_size[LLEXT_MEM_PREINIT], ext->mem[LLEXT_MEM_INIT],
				   ext->mem_size[LLEXT_MEM_INIT]);
		} else {
			/* cleanup functions from fini_array */
			memcpy(byte_ptr, ext->mem[LLEXT_MEM_FINI], ext->mem_size[LLEXT_MEM_FINI]);
		}

		/* Sanity check: pointers in this table must map inside the
		 * text region of the extension. If this fails, something went
		 * wrong during the relocation process.
		 * Using "char *" for these simplifies pointer arithmetic.
		 */
		const char *text_start = ext->mem[LLEXT_MEM_TEXT];
		const char *text_end = text_start + ext->mem_size[LLEXT_MEM_TEXT];
		const char **fn_ptrs = buf;

		for (int i = 0; i < (int)(table_size / sizeof(void *)); i++) {
			if (fn_ptrs[i] < text_start || fn_ptrs[i] >= text_end) {
				pr_err("%s function %i (%p) outside text region",
						is_init ? "bringup" : "teardown\n", i, fn_ptrs[i]);
				return -EFAULT;
			}
		}
	}

	return table_size;
}

