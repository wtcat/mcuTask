/*
 * Copyright 2025 wtcat
 */

#define TX_USE_BOARD_PRIVATE
#define TX_USE_SECTION_INIT_API_EXTENSION 1

#include "tx_api.h"

#include "basework/os/osapi.h"


#define MAIN_THREAD_PRIO  CONFIG_MAIN_THREAD_PRIO
#define MAIN_THREAD_STACK CONFIG_MAIN_THREAD_STACK

static TX_THREAD main_pid;
static ULONG main_stack[MAIN_THREAD_STACK / sizeof(ULONG)] __rte_section(".dtcm");

static void main_thread(void *arg);

int __rte_weak main(void) {
    return 0;
}

void tx_application_define(void *unused) {
    (void) unused;
    tx_thread_spawn(&main_pid, "main_thread", main_thread, &main_pid, main_stack, 
        sizeof(main_stack), MAIN_THREAD_PRIO, MAIN_THREAD_PRIO, TX_NO_TIME_SLICE, TX_AUTO_START);
}

static void main_thread(void *arg) {
    TX_THREAD *pid = arg;
    UINT old, new;

    /*
     * Do system and driver initialize
     */
    tx_thread_preemption_change(pid, 1, &new);
    do_sysinit();
    __do_init_array();
    tx_thread_preemption_change(pid, new, &old);

    /* Application entry */
    main();
}
