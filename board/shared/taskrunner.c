/*
 * Copyright (c) 2024 wtcat
 */

#include <errno.h>
#include "tx_api.h"

#include "basework/assert.h"

struct cancel_task {
    struct task task;
    TX_SEMAPHORE ack;
};

static void post_timeout_cb(ULONG id) {
    struct delayed_task *task = (struct delayed_task *)id;
    __task_post(task->base.runner, &task->base);
}

static void sync_helper_task(struct task *task) {
    struct cancel_task *ct = (struct cancel_task *)task;
    tx_semaphore_ceiling_put(&ct->ack, 1);
}

static void task_runner_thread(void *arg) {
    struct task_runner *runner = arg;
    struct task *curr, *next;
    void (*fn)(struct task *);
    TX_INTERRUPT_SAVE_AREA

    TX_DISABLE

    for ( ; ; ) {
        
        if (rte_list_empty(&runner->pending)) {
            /* 
             * If pending queue is empty then restore interrupt
             * and suspend task runner
             */
            TX_RESTORE
            tx_semaphore_get(&runner->sem, TX_WAIT_FOREVER);

            TX_DISABLE
        }

        /*
         * Walk around pending list and execute task callback
         */
        rte_list_foreach_entry_safe(curr, next, &runner->pending, node) {
            rte_list_del(&curr->node);
            runner->curr = curr;
            fn = curr->handler;
            TX_RESTORE

            fn(curr);

            TX_DISABLE
            runner->curr = NULL;
        }
    }

    TX_RESTORE
}

int __task_post(struct task_runner *runner, struct task *task) {
    scoped_guard(os_irq) {
        if (task->node.next != NULL)
            return -EBUSY;
        
        task->runner = runner;
        rte_list_add_tail(&task->node, &runner->pending);
        if (runner->curr)
            return 0;
    }
    return tx_semaphore_ceiling_put(&runner->sem, 1);
}

int __task_cancel(struct task *task, bool wait) {
    scoped_guard(os_irq) {
        if (task->node.next != NULL) {
            rte_list_del(&task->node);
            return 0;
        }
    }
    if (wait) {
        struct cancel_task ct;

        init_task(&ct.task, sync_helper_task);
        tx_semaphore_create(&ct.ack, "task_cancel", 0);
        __task_post(task->runner, &ct.task);
        tx_semaphore_get(&ct.ack, TX_WAIT_FOREVER);
        tx_semaphore_delete(&ct.ack);
    }
    return 0;
}

int __delayed_task_post(struct task_runner *runner, struct delayed_task *task, 
    unsigned long ticks) {
    if (ticks == TX_NO_WAIT)
        return __task_post(runner, &task->base);

    tx_timer_deactivate(&task->timer);
    __task_cancel(&task->base, false);
    task->base.runner = runner;
    task->timer.tx_timer_internal.tx_timer_internal_remaining_ticks = ticks;
    return (int)tx_timer_activate(&task->timer);
}

int __delayed_task_cancel(struct delayed_task *task, bool wait) {
    tx_timer_deactivate(&task->timer);
    return __task_cancel(&task->base, wait);
}

int task_post(struct task_runner *runner, struct task *task) {
    rte_assert(runner != NULL);
    if (task == NULL)
        return -EINVAL;
    
    return __task_post(runner, task);
}

int delayed_task_post(struct task_runner *runner, struct delayed_task *task, 
    unsigned long ticks) {
    rte_assert(runner != NULL);
    if (task == NULL)
        return -EINVAL;

    return __delayed_task_post(runner, task, ticks);
}

int task_cancel(struct task *task, bool wait) {
    if (task == NULL)
        return -EINVAL;
    return __task_cancel(task, wait);
}

int delayed_task_cancel(struct delayed_task *task, bool wait) {
    if (task == NULL)
        return -EINVAL;
    return __delayed_task_cancel(task, wait);
}

void init_task(struct task *task, void (*handler)(struct task *)) {
    task->node = (struct rte_list){NULL, NULL};
    task->handler = handler;
}

void init_delayed_task(struct delayed_task *task, void (*handler)(struct task *)) {
    init_task(&task->base, handler);
    tx_timer_create(&task->timer, "delayed_task",
            post_timeout_cb, (ULONG)task,
            0, 0, TX_FALSE);
}

int task_runner_construct(struct task_runner *runner, const char *name, void *stack, 
    size_t stack_size, unsigned int prio, int cpu) {
    
    (void) cpu;
    if (runner == NULL)
        return -EINVAL;

    if (stack == NULL)
        return -EINVAL;

    if (stack_size < TX_MINIMUM_STACK)
        return -EINVAL;

    if (prio >= TX_MAX_PRIORITIES)
        return -EINVAL;

    if (name == NULL)
        name = "task-runner";

    RTE_INIT_LIST(&runner->pending);
    tx_semaphore_create(&runner->sem, (CHAR *)name, 0);
    tx_thread_spawn(&runner->pid, name, task_runner_thread, runner, stack, 
        stack_size, prio, prio, TX_NO_TIME_SLICE, TX_AUTO_START);
    
    return 0;
}

/*
 * Create task runner for system
 */

#ifndef TX_TASK_RUNNER_STACK_SIZE
#define TX_TASK_RUNNER_STACK_SIZE TX_MINIMUM_STACK
#endif
#ifndef TX_TASK_RUNNER_PRIO
#define TX_TASK_RUNNER_PRIO (TX_MAX_PRIORITIES / 2)
#endif

#if TX_TASK_RUNNER_STACK_SIZE > 0
struct task_runner _system_taskrunner __fastbss;
static char stack_memory[TX_TASK_RUNNER_STACK_SIZE] __fastbss __rte_aligned(8);

static int task_runner_init(void) {
    return task_runner_construct(&_system_taskrunner, "system_taskrunner", 
        stack_memory, sizeof(stack_memory), TX_TASK_RUNNER_PRIO, 0);
}

SYSINIT(task_runner_init, SI_MEMORY_LEVEL, 80);
#endif /* TX_TASK_RUNNER_STACK_SIZE > 0 */
