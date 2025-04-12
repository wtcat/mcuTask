/*
 * Copyright 2024 wtcat
 */
#include <stdarg.h>
#include <tx_api.h>

#include <base/lib/iovpr.h>

struct printk_buffer {
#define BUF_SIZE 256
	char buf[BUF_SIZE + 2];
	uint16_t len;
};

static void empty_puts(const char *s, size_t len) {
	(void) s;
	(void) len;
}

static void __fastcode put_char(int c, void *arg) {
	struct printk_buffer *p = (struct printk_buffer *)arg;
	if (rte_likely(p->len < BUF_SIZE)) {
		if (c == '\n')
			p->buf[p->len++] = '\r';
		p->buf[p->len++] = (char)c;
	}
}

int vprintk(const char *fmt, va_list ap) {
	struct printk_buffer pb;

	pb.len = 0;
	int len = _IO_Vprintf(put_char, &pb, fmt, ap);
	if (pb.len > 0)
		__console_puts(pb.buf, pb.len);

	return len;
}

int printk(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	int len = vprintk(fmt, ap);
	va_end(ap);
	return len;
}

console_puts_t __console_puts = empty_puts;

static int platform_setup(void) {
	return 0;
}

SYSINIT(platform_setup, SI_EARLY_LEVEL, 10);
