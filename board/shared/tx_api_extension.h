/*
 * Copyright 2024 wtcat
 */

#ifndef TX_API_EXTENSION_H_
#define TX_API_EXTENSION_H_

#include <stdarg.h>
#include <sys/types.h>

#include <base/compiler_attributes.h>
#include <base/generic.h>
#include <base/linker.h>
#include <base/container/queue.h>
#include <base/container/list.h>
#include <base/hrtimer_.h>

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

#define __externC extern"C"

/*
 * Thread extension API
 */
#if TX_USE_KERNEL_API_EXTENSION
#include <base/cleanup.h>

#define tx_thread_spawn(a, b, c, d, e, f, g, h, i, j) \
    tx_thread_create((a), (CHAR *)(b), (VOID(*)(ULONG))(void *)(c), (ULONG)(d), (e), (f), (g), (h), (i), (j))

#define TX_THREAD_STACK_DEFINE(_name, _size, ...) \
    ULONG _name[rte_div_roundup((_size), sizeof(ULONG))] __VA_ARGS__;

#define TX_THREAD_DEFINE(_name, _size, ...) \
    struct __VA_ARGS__ {   \
        TX_THREAD task;  \
        char stack[_size]; \
    } _name;


UINT tx_os_nanosleep(uint64_t time);
UINT tx_os_delay(uint64_t nano_sec);
void tx_thread_foreach(bool (*iterator)(TX_THREAD *, void *arg), void *arg) __rte_nonnull(1);

/*
 * Define lock guard
 */
#ifndef __cplusplus
#ifdef TX_DISABLE_ERROR_CHECKING
DEFINE_GUARD(os_mutex, TX_MUTEX *, _tx_mutex_get(_T, 0xFFFFFFFFUL), _tx_mutex_put(_T))
#else
DEFINE_GUARD(os_mutex, TX_MUTEX *, _txe_mutex_get(_T, 0xFFFFFFFFUL), _txe_mutex_put(_T))
#endif /* TX_DISABLE_ERROR_CHECKING */


#if defined(__linux__)
DEFINE_LOCK_GUARD_0(os_irq, (_T)->key = _tx_thread_interrupt_disable(), \
    _tx_thread_interrupt_restore((_T)->key), unsigned int key)

#else
DEFINE_LOCK_GUARD_0(os_irq, (_T)->key = __disable_interrupts(), \
    __restore_interrupt((_T)->key), unsigned int key)
#endif /* __linux__ */
#endif /* __cplusplus */
#endif /* TX_USE_KERNEL_API_EXTENSION */

/*
 * Simple object pool 
 */
struct object_pool {
    void *free_chain;
};

void *object_allocate(struct object_pool *pool);
void object_free(struct object_pool *pool, void *obj);
int  object_pool_initialize(struct object_pool *pool, void *buffer, size_t size, 
    size_t objsize);

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

/* 
 * System initialize level (must be <= 99)
 */
#define SI_EARLY_LEVEL         5
#define SI_MEMORY_LEVEL        10
#define SI_PREDRIVER_LEVEL     60
#define SI_BUSDRIVER_LEVEL     70
#define SI_DRIVER_LEVEL        80
#define SI_FILESYSTEM_LEVEL    90
#define SI_APPLICATION_LEVEL   99

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

/*
 * Console interface
 */
typedef void (*console_puts_t)(const char *, size_t);
extern console_puts_t __console_puts;


/*
 * Global variable
 */
extern char _isr_stack_area[CONFIG_ISR_STACK_SIZE];

#ifdef __cplusplus
}
#endif
#endif /* TX_API_EXTENSION_H_ */
