/*
 * Copyright 2025 wtcat
 */
#ifndef SERVICE_PRINTK_H_
#define SERVICE_PRINTK_H_

#include <stdarg.h>
#include <stddef.h>
#include <base/compiler.h>

#ifdef __cplusplus
extern "C"{
#endif

/*
 * Platform interface
 */
int printk(const char *fmt, ...) __rte_printf(1, 2);
int vprintk(const char *fmt, va_list ap);

/*
 * Console interface
 */
typedef void (*console_puts_t)(const char *, size_t);
extern console_puts_t __console_puts;

#ifdef __cplusplus
}
#endif
#endif /* SERVICE_PRINTK_H_ */
