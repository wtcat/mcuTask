/*
 * Copyright 2022 wtcat
 */
#ifndef BASE_PRINTER_H_
#define BASE_PRINTER_H_

#include <stddef.h>
#include <stdarg.h>

#include "base/compiler.h"

#ifdef __cplusplus
extern "C"{
#endif
struct circ_buffer;

struct printer {
    int (*format)(void *context, const char *fmt, va_list ap);
    void *context;
};

/*
 * virt_format - Virtual format output
 * @printer: printer object
 * @fmt: format string
 * return the number of bytes that has been output
 */
static inline int virt_format(struct printer *printer, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int len = printer->format(printer->context, fmt, ap);
    va_end(ap);
    return len;
}

#ifdef __cplusplus
}
#endif
#endif /* BASE_PRINTER_H_ */
