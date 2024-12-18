/*
 * Copyright (c) 2024 wtcat
 */

#include <errno.h>
#include "tx_api.h"

#include "basework/container/list.h"
#include "basework/assert.h"

struct task_runner {
    TX_THREAD thread;
    TX_SEMAPHORE sem;
    struct rte_list pending;
    struct task *curr;
};

struct task {
    struct rte_list node;
    void (*handler)(struct task *);
};

struct delayed_task {
    struct task base;
    TX_TIMER timer;
    struct task_runner *runner;
};

static void task_runner_thread(void *arg) {
    struct task_runner *runner = arg;
    struct task *curr, *next;
    struct rte_list head;
    void (*fn)(struct task *);

    for ( ; ; ) {
        scoped_guard(os_irq) {
            if (rte_list_empty(&runner->pending)) {
                tx_semaphore_get(&runner->sem, TX_WAIT_FOREVER);
                continue;
            }
            RTE_INIT_LIST(&head);
            __rte_list_splice(&runner->pending, head.prev, &head);
        }

        rte_list_foreach_entry_safe(curr, next, &head, node) {
            runner->curr = curr;
            fn = curr->handler;
            rte_list_del(&curr->node);

            fn(curr);
            runner->curr = NULL;
        }
    }
}

int __task_post(struct task_runner *runner, struct task *task) {
    scoped_guard(os_irq) {
        if (task->node.next != NULL)
            return -EBUSY;

        rte_list_add_tail(&task->node, &runner->pending);
        if (runner->curr)
            return 0;
    }
    return tx_semaphore_ceiling_put(&runner->sem, 1);
}

int task_post(struct task_runner *runner, struct task *task) {
    rte_assert(runner != NULL);
    if (task == NULL)
        return -EINVAL;
    
    return __task_post(runner, task);
}

static void post_timeout_cb(ULONG id) {
    struct delayed_task *task = (struct delayed_task *)id;
    __task_post(task->runner, &task->base);
}

int task_delayed_post(struct task_runner *runner, struct delayed_task *task, 
    unsigned long ticks) {
    rte_assert(runner != NULL);
    if (task == NULL)
        return -EINVAL;

    if (ticks == TX_NO_WAIT)
        return __task_post(runner, &task->base);

    tx_timer_deactivate(&task->timer);
    task->timer.tx_timer_internal.tx_timer_internal_remaining_ticks = ticks;
    return (int)tx_timer_activate(&task->timer);
}

void init_task(struct task *task, void (*handler)(struct task *)) {
    task->handler = handler;
}

void init_delayed_task(struct delayed_task *task, void (*handler)(struct task *)) {
    task->base.handler = handler;
    tx_timer_create(&task->timer, "delayed_task",
            post_timeout_cb, (ULONG)task,
            0, 0, TX_FALSE);
}

