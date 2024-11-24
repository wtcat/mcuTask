/*
 * Copyright 2024 wtcat
 */
#include <stdarg.h>
#include "tx_api.h"

#include "basework/lib/iovpr.h"

#ifndef __fastcode
#define __fastcode
#endif

static void __fastcode put_char(int c, void *arg) {
	console_putc((char)c);
	if (c == '\n')
		console_putc('\r');
}

int printk(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	int len = _IO_Vprintf(put_char, NULL, fmt, ap);
	va_end(ap);

	return len;
}
