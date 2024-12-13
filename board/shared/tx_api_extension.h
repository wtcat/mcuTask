/*
 * Copyright 2024 wtcat
 */

#ifndef TX_API_EXTENSION_H_
#define TX_API_EXTENSION_H_

#include <stdarg.h>
#include <sys/types.h>

#include "basework/generic.h"
#include "basework/linker.h"
#include "basework/container/queue.h"
#include "basework/hrtimer_.h"

#ifdef __cplusplus
extern "C"{
#endif

#ifndef TX_USE_DEVICE_API_EXTENSION
#define TX_USE_DEVICE_API_EXTENSION 1
#endif
#ifndef TX_USE_KERNEL_API_EXTENSION
#define TX_USE_KERNEL_API_EXTENSION 1
#endif
#ifndef TX_USE_SECTION_INIT_API_EXTENSION
#define TX_USE_SECTION_INIT_API_EXTENSION 0
#endif
#ifndef TX_USE_USE_SYSINIT_API_EXTENSION
#define TX_USE_USE_SYSINIT_API_EXTENSION 1
#endif

/*
 * Thread extension API
 */
#if TX_USE_KERNEL_API_EXTENSION
#include "basework/cleanup.h"

#define tx_thread_spawn(a, b, c, d, e, f, g, h, i, j) \
    tx_thread_create((a), (b), (VOID(*)(ULONG))(void *)(c), (ULONG)(d), (e), (f), (g), (h), (i), (j))

#define TX_THREAD_DEFINE(_name, _size, ...) \
    struct __VA_ARGS__ {   \
        TX_THREAD task;  \
        char stack[_size]; \
    } _name;


UINT tx_os_nanosleep(uint64_t time);
UINT tx_os_delay(uint64_t nano_sec);


/*
 * Define lock guard
 */
DEFINE_GUARD(os_mutex, TX_MUTEX *, tx_mutex_get(_T, 0xFFFFFFFFUL), tx_mutex_put(_T))
DEFINE_LOCK_GUARD_0(os_irq, (_T)->key = __disable_interrupts(), \
    __restore_interrupt((_T)->key), unsigned int key)	
#endif /* TX_USE_KERNEL_API_EXTENSION */


/*
 * Device class definition
 */
#if TX_USE_DEVICE_API_EXTENSION
#define DEVICE_CLASS_DEFINE(_type, ...) \
    struct _type { \
        const char *name; \
        STAILQ_ENTRY(device) link; \
        __VA_ARGS__ \
    }

/* 
 * Device structure
 */
DEVICE_CLASS_DEFINE(device);

/* 
 * Device helper interface
 */
#define dev_extension(_dev, _type) (_type *)((_dev) + 1)
#define to_devclass(p) (struct device *)(p)

struct device *device_find(const char *name);
int  device_register(struct device *dev);
int  device_unregister(struct device *dev);
void device_foreach(bool (*iterator)(struct device *, void *), void *user);
#endif /* TX_USE_DEVICE_API_EXTENSION */

/*
 * Device I/O options
 */
#define O_NOBLOCK 0x0001

/*
 * Custom define section
 */
#ifndef __fastcode
#define __fastcode
#endif
#ifndef __fastbss
#define __fastbss
#endif
#ifndef __fastdata
#define __fastdata
#endif

#if TX_USE_SECTION_INIT_API_EXTENSION
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
#endif /* TX_USE_SECTION_INIT_API_EXTENSION */

/*
 * System intitialize 
 */
#if TX_USE_USE_SYSINIT_API_EXTENSION
struct sysinit_item {
   int (*handler)(void);
   const char *name;
};

/* System initialize level */
#define SI_EARLY_LEVEL         5
#define SI_MEMORY_LEVEL        10
#define SI_PREDRIVER_LEVEL     70
#define SI_DRIVER_LEVEL        80
#define SI_APPLICATION_LEVEL   90

#define SYSINIT(_handler, _level, _order) \
    __SYSINIT(_handler, _level, _order)

#define __SYSINIT(_handler, _level, _order) \
    ___SYSINIT(_handler, 0x##_level##_order)

#define ___SYSINIT(_handler, _order) \
    enum { __enum_##_handler = _order}; \
    static LINKER_ROSET_ITEM_ORDERED(sysinit, struct sysinit_item, \
        _handler, _order) = { \
        .handler = _handler, \
        .name = #_handler \
   }

void do_sysinit(void);
#endif /* TX_USE_USE_SYSINIT_API_EXTENSION */
/*
 * Memory allocate interface
 */
#define GMF_KERNEL 0x0001
#define GMF_WAIT   0x0002

void *__kmalloc(size_t size, unsigned int gmf);
void *__kzalloc(size_t size, unsigned int gmf);
void  __kfree(void *ptr);

#define kmalloc(s, f) __kmalloc(s, f)
#define kzalloc(s, f) __kzalloc(s, f)
#define kfree(p)      __kfree(p)

/*
 * Platform interface
 */
int printk(const char *fmt, ...) __rte_printf(1, 2);
int vprintk(const char *fmt, va_list ap);

int init_irq(void);
int enable_irq(int irq);
int disable_irq(int irq);
void dispatch_irq(void);
int request_irq(int irq, void (*handler)(void *), void *arg);
int remove_irq(int irq, void (*handler)(void *), void *arg);
int gpio_request_irq(uint32_t gpio, void (*fn)(int line, void *arg), void *arg,
					 bool rising_edge, bool falling_edge);
int gpio_remove_irq(uint32_t gpio, void (*fn)(int line, void *arg), void *arg);

/*
 * Console interface
 */
typedef void (*console_puts_t)(const char *, size_t);
typedef int  (*console_getc_t)(void);

extern console_puts_t __console_puts;
extern console_getc_t __console_getc;


#ifdef __cplusplus
}
#endif

#include "drivers/uart.h"

#endif /* TX_API_EXTENSION_H_ */
