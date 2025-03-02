/*
 * Copyright 2025 wtcat
 */

#include "tx_api.h"

#include "subsys/cli/cli.h"
#include "subsys/fs/fs.h"


static TX_THREAD main_pid;
static TX_THREAD_STACK_DEFINE(main_stack, CONFIG_MAIN_THREAD_STACK_SIZE);

static void main_thread(void *arg) {
    TX_THREAD *pid = tx_thread_identify();
    UINT old, new;

    /*
     * Do system and driver initialize
     */
    tx_thread_preemption_change(pid, 1, &new);
    do_sysinit();
    __do_init_array();
    tx_thread_preemption_change(pid, new, &old);
    
    /* Mount file system */
    if (!fs_mount(&main_fs))
        printk("Mount filesystem success\n");
    
    /*
     * Create command line interface
     */
    static ULONG stack[1024];
    cli_run("uart1", 15, stack, sizeof(stack), 
        "[task]# ", &_cli_ifdev_uart);

}
