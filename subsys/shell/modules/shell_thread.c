/*
 * Copyright 2025 wtcat
 */

#define TX_SOURCE_CODE
#include <inttypes.h>
#include <tx_api.h>
#include <tx_thread.h>

#include <base/lib/string.h>
#include <subsys/shell/shell.h>

#define THREAD_MONITOR_MAX 32

#ifdef TX_EXECUTION_PROFILE_ENABLE

struct thread_monitor {
    EXECUTION_TIME thread_execution_time;
    CHAR *thread_name;
    UINT thread_priority;
    UINT thread_current_priority;
    UINT thread_state;
};

struct thread_param {
    struct thread_monitor *array;
    UINT count;
    UINT index;
};

struct stack_monitor {
    CHAR *thread_name;
    VOID *thread_stack_start;
    VOID *thread_stack_end;
    UINT thread_stack_size;
};

struct stack_param {
    struct stack_monitor *array;
    UINT count;
    UINT index;
};

#define _STATE_ITEM(_name) [TX_##_name] = #_name
static const char *thread_state_name[] = {
	_STATE_ITEM(READY),
	_STATE_ITEM(COMPLETED),
	_STATE_ITEM(TERMINATED),
	_STATE_ITEM(SUSPENDED),
	_STATE_ITEM(SLEEP),
	_STATE_ITEM(QUEUE_SUSP),
	_STATE_ITEM(SEMAPHORE_SUSP),
	_STATE_ITEM(EVENT_FLAG),
	_STATE_ITEM(BLOCK_MEMORY),
	_STATE_ITEM(BYTE_MEMORY),
	_STATE_ITEM(IO_DRIVER),
	_STATE_ITEM(FILE),
	_STATE_ITEM(TCP_IP),
	_STATE_ITEM(MUTEX_SUSP),
	_STATE_ITEM(PRIORITY_CHANGE),
};

static void thread_foreach(bool (*iterator)(TX_THREAD *, void *arg), void *arg) {
    TX_INTERRUPT_SAVE_AREA
    TX_THREAD *thread_ptr;
    UINT thread_count;

    TX_DISABLE
    _tx_thread_preempt_disable++;
    TX_RESTORE

    thread_ptr   = _tx_thread_created_ptr;
    thread_count = _tx_thread_created_count;

    while (thread_count > 0) {
        if (iterator(thread_ptr, arg))
            break;

        thread_count--;

        /* Pointer to the next thread */
        thread_ptr = thread_ptr->tx_thread_created_next;
    }
    
    TX_DISABLE
    _tx_thread_preempt_disable--;
    TX_RESTORE
}

static bool cpuuse_iterator(TX_THREAD *thread_ptr, void *arg) {
    struct thread_param *p = arg;
    UINT index = p->index;

    if (index >= p->count)
        return true;

    p->array[index].thread_execution_time = 
        thread_ptr->tx_thread_execution_time_total;
    p->array[index].thread_name = thread_ptr->tx_thread_name;
    p->array[index].thread_priority = thread_ptr->tx_thread_user_priority;
    p->array[index].thread_current_priority = thread_ptr->tx_thread_priority;
    p->array[index].thread_state = thread_ptr->tx_thread_state;
    p->index = index + 1;

    return false;
}

static UINT thread_information_collect(struct thread_monitor *thread_monitor, 
    size_t count) {
    struct thread_param param;

    param.array = thread_monitor;
    param.count = count;
    param.index = 0;

    thread_foreach(cpuuse_iterator, &param);
    return param.index;
}

static int cli_thread_monitor(const struct shell *sh, int argc, char *argv[]) {
    if (argc == 1) {
        struct thread_monitor monitors[THREAD_MONITOR_MAX];
        EXECUTION_TIME total_time, isr_time, idle_time;
        UINT threads;

        threads = thread_information_collect(monitors, THREAD_MONITOR_MAX);
        _tx_execution_thread_total_time_get(&total_time);
        _tx_execution_isr_time_get(&isr_time);
        _tx_execution_idle_time_get(&idle_time);

        shell_fprintf(sh, SHELL_NORMAL, 
            "\nThreadTotalTime: %" PRIu64 " InterruptTime: %" PRIu64 " IdleTime: %" PRIu64 "\n",
            total_time, isr_time, idle_time
        );
        shell_fprintf(sh, SHELL_NORMAL,
        "\n"
            " NAME                | RPRI | CPRI   | STATE               | PERCENT \n"
            "---------------------+---------------+---------------------+---------\n"
        );

        for (UINT i = 0; i < threads; i++) {
            CHAR thread_name[20];

            if (total_time == 0)
                total_time = 1;

            UINT percent = monitors[i].thread_execution_time * 100 / total_time;
            strlcpy(thread_name, monitors[i].thread_name, 19);
            shell_fprintf(sh, SHELL_NORMAL, 
                " %-19s |  %3u"  " |  %3u"  "   |  %-19s |  %u""%% \n",
                thread_name,
                monitors[i].thread_priority,
                monitors[i].thread_current_priority,
                thread_state_name[monitors[i].thread_state],
                percent
            );
        }
        return 0;
    }
    
    if (argc == 2) {
        /* Reset time */
        if (!strcmp(argv[1], "-r")) {
            TX_INTERRUPT_SAVE_AREA
            _tx_execution_thread_total_time_reset();

            TX_DISABLE
            _tx_execution_isr_time_reset();
            _tx_execution_idle_time_reset();
            TX_RESTORE
            return 0;
        }
    }

    shell_fprintf(sh, SHELL_NORMAL, "cpuuse [-r]\n");
    return -EINVAL;
}

static bool stackuse_iterator(TX_THREAD *thread_ptr, void *arg) {
    struct stack_param *p = arg;
    UINT index = p->index;

    if (index >= p->count)
        return true;

    p->array[index].thread_name = thread_ptr->tx_thread_name;
    p->array[index].thread_stack_start = thread_ptr->tx_thread_stack_start;
    p->array[index].thread_stack_end = thread_ptr->tx_thread_stack_end;
    p->array[index].thread_stack_size = thread_ptr->tx_thread_stack_size;
    p->index = index + 1;

    return false;
}

static UINT stack_information_collect(struct stack_monitor *stack_monitor, 
    size_t count) {
    struct stack_param param;

    param.array = stack_monitor;
    param.count = count;
    param.index = 0;

    thread_foreach(stackuse_iterator, &param);
    return param.index;
}

static int cli_stack_monitor(const struct shell *sh, int argc, char *argv[]) {
    if (argc != 1) {
        shell_fprintf(sh, SHELL_NORMAL, "stackuse\n");
        return -EINVAL;
    }

    struct stack_monitor monitors[THREAD_MONITOR_MAX];
    UINT stack_size = 0;
    UINT threads;

    threads = stack_information_collect(monitors, THREAD_MONITOR_MAX);
    for (UINT i = 0; i < threads; i++)
        stack_size += monitors[i].thread_stack_size;

    shell_fprintf(sh, SHELL_NORMAL, "\nThreadStackSize: %u"  " bytes\n", stack_size);
    shell_fprintf(sh, SHELL_NORMAL,
    "\n"
        " NAME                | STACK AREA             | SIZE    \n"
        "---------------------+------------------------+---------\n"
    );

    for (UINT i = 0; i < threads; i++) {
        CHAR thread_name[20];
        strlcpy(thread_name, monitors[i].thread_name, 19);

        shell_fprintf(sh, SHELL_NORMAL, 
            " %-19s |  %08x"  " - %08x"  "   |  %u" " \n",
            thread_name,
            (uintptr_t)monitors[i].thread_stack_start,
            (uintptr_t)monitors[i].thread_stack_end,
            (size_t)monitors[i].thread_stack_size
        );
    }

    return 0;
}

static int tx_profile_init(void) {
    _tx_execution_initialize();
    _tx_execution_thread_total_time_reset();
    _tx_execution_isr_time_reset();
    _tx_execution_idle_time_reset();
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(kernel_cmds, 
	SHELL_CMD_ARG(cpuuse, NULL, "Show cpu usage and task information", cli_thread_monitor, 0, 0),
    SHELL_CMD_ARG(stackuse, NULL, "Show thread stack usage", cli_stack_monitor, 0, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(kernel, &kernel_cmds, "Kernel commands", NULL);

SYSINIT(tx_profile_init, SI_APPLICATION_LEVEL, 50);
#endif /* TX_EXECUTION_PROFILE_ENABLE */