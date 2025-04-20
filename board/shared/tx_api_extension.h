/*
 * Copyright 2024 wtcat
 */

#ifndef TX_API_EXTENSION_H_
#define TX_API_EXTENSION_H_

#include <base/container/list.h>
#include <base/cleanup.h>
#include <base/hrtimer_.h>

#ifdef __cplusplus
extern "C"{
#endif


#define __externC extern"C"

/*
 * Thread extension API
 */
#define tx_thread_spawn(a, b, c, d, e, f, g, h, i, j) \
    tx_thread_create((a), (CHAR *)(b), (VOID(*)(ULONG))(void *)(c), (ULONG)(d), (e), (f), (g), (h), (i), (j))

void tx_thread_foreach(bool (*iterator)(TX_THREAD *, void *arg), void *arg) __rte_nonnull(1);
UINT tx_os_nanosleep(uint64_t time);
UINT tx_os_delay(uint64_t nano_sec);


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

/*
 * Global variable
 */
extern char _isr_stack_area[CONFIG_ISR_STACK_SIZE];

#ifdef __cplusplus
}
#endif
#endif /* TX_API_EXTENSION_H_ */
