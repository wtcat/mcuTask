/*
 * Copyright 2025 wtcat
 */
#ifndef SERVICE_START_H_
#define SERVICE_START_H_

#ifdef __cplusplus
extern "C"{
#endif

#define LINKER_SYMBOL(_s) extern char _s[];

/* Clear bss section */
#define _clear_bss_section(_start, _end) \
do { \
	extern char _start[]; \
	extern char _end[]; \
	for (uint32_t *dest = (uint32_t *)_start; dest < (uint32_t *)_end;) \
		*dest++ = 0; \
} while (0)

/* Initialize data section */
#define _copy_data_section(_start, _end, _loadstart) \
do { \
	extern char _start[]; \
	extern char _end[]; \
	extern char _loadstart[]; \
	for (uint32_t *src = (uint32_t *)_loadstart, *dest = (uint32_t *)_start; \
		 dest < (uint32_t *)_end;) \
		*dest++ = *src++; \
} while (0)

void __do_init_array(void);

#ifdef __cplusplus
}
#endif
#endif /* SERVICE_START_H_ */
