/*
 * Copyright 2024 wtcat
 */

#define TX_SOURCE_CODE
#include <inttypes.h>
#include "tx_api.h"
#include "tx_thread.h"
#include "subsys/cli/cli.h"

#define THREAD_MONITOR_MAX 32

#ifdef TX_EXECUTION_PROFILE_ENABLE

struct thread_monitor {
    EXECUTION_TIME thread_execution_time;
    CHAR *thread_name;
    UINT thread_priority;
    UINT thread_current_priority;
    UINT thread_state;
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

static UINT thread_information_collect(struct thread_monitor *thread_monitor, 
    size_t count) {
    TX_INTERRUPT_SAVE_AREA
    TX_THREAD *thread_ptr;
    UINT thread_count;
    UINT index = 0;

    TX_DISABLE
    _tx_thread_preempt_disable++;
    TX_RESTORE

    thread_ptr   = _tx_thread_created_ptr;
    thread_count = _tx_thread_created_count;

    while (thread_count > 0 && index < count) {
        thread_monitor[index].thread_execution_time = 
            thread_ptr->tx_thread_execution_time_total;
        thread_monitor[index].thread_name = thread_ptr->tx_thread_name;
        thread_monitor[index].thread_priority = thread_ptr->tx_thread_user_priority;
        thread_monitor[index].thread_current_priority = thread_ptr->tx_thread_priority;
        thread_monitor[index].thread_state = thread_ptr->tx_thread_state;

        index++;
        thread_count--;

        /* Pointer to the next thread */
        thread_ptr = thread_ptr->tx_thread_created_next;
    }
    
    TX_DISABLE
    _tx_thread_preempt_disable--;
    TX_RESTORE

    return index;
}

static int cli_thread_monitor(struct cli_process *cli, int argc, char *argv[]) {
    if (argc == 1) {
        struct thread_monitor monitors[THREAD_MONITOR_MAX];
        EXECUTION_TIME total_time, isr_time, idle_time;
        UINT threads;

        threads = thread_information_collect(monitors, sizeof(monitors));
        _tx_execution_thread_total_time_get(&total_time);
        _tx_execution_isr_time_get(&isr_time);
        _tx_execution_idle_time_get(&idle_time);

        cli_println(cli, 
            "\nThreadTotalTime: %" PRIu32 " InterruptTime: %" PRIu32 " IdleTime: %" PRIu32 "\n",
            total_time, isr_time, idle_time
        );
        cli_println(cli,
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
            cli_println(cli, 
                " %-19s |  %3" PRId32 " |  %3" PRId32 "   |  %-19s |  %"PRId32"%% \n",
                thread_name,
                monitors[i].thread_priority,
                monitors[i].thread_current_priority,
                thread_state_name[monitors[i].thread_state],
                percent
            );
        }
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
        }
    }
    return 0;
}

CLI_CMD(cpuuse, "cpuuse [-r]",
    "Show cpu usage and task information",
    cli_thread_monitor
)

static int tx_profile_init(void) {
    _tx_execution_initialize();
    _tx_execution_thread_total_time_reset();
    _tx_execution_isr_time_reset();
    _tx_execution_idle_time_reset();
    return 0;
}

SYSINIT(tx_profile_init, SI_APPLICATION_LEVEL, 50);
#endif /* TX_EXECUTION_PROFILE_ENABLE */
