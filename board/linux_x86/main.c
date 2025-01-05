/*
 * Copyright 2024 wtcat
 */

#include "tx_api.h"

#define MAIN_THREAD_PRIO  11
#define MAIN_THREAD_STACK 4096

#define KERNEL_HEAP_SIZE 1024*1024
char _kernel_byte_pool_start[KERNEL_HEAP_SIZE];
UINT _kernel_byte_pool_size = KERNEL_HEAP_SIZE;

static TX_THREAD main_pid;
static ULONG main_stack[MAIN_THREAD_STACK / sizeof(ULONG)];



int main(int argc, char *argv[]) {

    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();
    return 0;
}

static void main_thread(void *arg) {
    TX_THREAD *pid = &main_pid;
    UINT old, new;

    (void) arg;
    /*
     * Do system and driver initialize
     */
    tx_thread_preemption_change(pid, 1, &new);
    do_sysinit();
    tx_thread_preemption_change(pid, new, &old);

    for ( ; ; ) {
        tx_thread_sleep(TX_MSEC(10000));
    }
}

void tx_application_define(void *unused) {
    tx_thread_spawn(&main_pid, "main_thread", main_thread, &main_pid, main_stack, 
        sizeof(main_stack), MAIN_THREAD_PRIO, MAIN_THREAD_PRIO, TX_NO_TIME_SLICE, TX_AUTO_START);
}
