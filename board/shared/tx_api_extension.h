/*
 * Copyright 2024 wtcat
 */

#ifndef TX_API_EXTENSION_H_
#define TX_API_EXTENSION_H_

#include <stdarg.h>
#include <sys/_intsup.h>
#include <sys/types.h>

#include "basework/compiler_attributes.h"
#include "basework/generic.h"
#include "basework/linker.h"
#include "basework/container/queue.h"
#include "basework/container/list.h"
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
    tx_thread_create((a), (CHAR *)(b), (VOID(*)(ULONG))(void *)(c), (ULONG)(d), (e), (f), (g), (h), (i), (j))

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
 * Task Runner
 */
struct task_runner {
    TX_THREAD pid;
    TX_SEMAPHORE sem;
    struct rte_list pending;
    struct task *curr;
};

struct task {
    struct rte_list node;
    void (*handler)(struct task *);
    struct task_runner *runner;
};

struct delayed_task {
    struct task base;
    TX_TIMER timer;
};

#define to_delayedtask(_task) \
    rte_container_of(_task, struct delayed_task, base)

extern struct task_runner _system_taskrunner;

int __task_post(struct task_runner *runner, struct task *task);
int task_post(struct task_runner *runner, struct task *task);
int __delayed_task_post(struct task_runner *runner, struct delayed_task *task, 
    unsigned long ticks);
int delayed_task_post(struct task_runner *runner, struct delayed_task *task, 
    unsigned long ticks);
int __task_cancel(struct task *task, bool wait);
int task_cancel(struct task *task, bool wait);
int __delayed_task_cancel(struct delayed_task *task, bool wait);
int delayed_task_cancel(struct delayed_task *task, bool wait);
void init_task(struct task *task, void (*handler)(struct task *));
void init_delayed_task(struct delayed_task *task, void (*handler)(struct task *));
int task_runner_construct(struct task_runner *runner, const char *name, void *stack, 
    size_t stack_size, unsigned int prio, int cpu);

#define ktask_post(_task) \
    task_post(&_system_taskrunner, (_task))

#define delayed_ktask_post(_task, _delay) \
    delayed_task_post(&_system_taskrunner, (_task), (_delay))



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

void __do_init_array(void);
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
#define SI_PREDRIVER_LEVEL     60
#define SI_BUSDRIVER_LEVEL     70
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

/*
 * Device Driver
 */
#include "drivers/device.h"
#include "drivers/uart.h"

#ifdef __cplusplus
}
#endif
#endif /* TX_API_EXTENSION_H_ */
