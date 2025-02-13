/*
 * Copyright 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE
#define TX_USE_SECTION_INIT_API_EXTENSION 1

#include "tx_api.h"
#include "fx_api.h"

#include "basework/rte_cpu.h"
#include "basework/os/osapi.h"

#include "subsys/cli/cli.h"
#include "subsys/fs/fs.h"

#define MAIN_THREAD_PRIO  11
#define MAIN_THREAD_STACK 1024

static TX_THREAD main_pid;
static ULONG main_stack[MAIN_THREAD_STACK / sizeof(ULONG)] __rte_section(".dtcm");
static FX_MEDIA fs_media;
static struct fs_class main_fs = {
    .mnt_point = "/c",
    .type = FS_EXFATFS,
    .storage_dev = "sdblk0",
    .fs_data = &fs_media
};

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

    /* Mount file system */
    fs_mount(&main_fs);
    
    /*
     * Create command line interface
     */
    static ULONG stack[1024];
    cli_run("uart1", 15, stack, sizeof(stack), 
        "[task]# ", &_cli_ifdev_uart);

    //TODO: Test code
    // int count = 0;
    // uint32_t prevalue = 0;
    for ( ; ; ) {
        // uint32_t now = HRTIMER_JIFFIES;
        // uint32_t diff = HRTIMER_CYCLE_TO_US(now - prevalue);
        // prevalue = now;
        // printk("Thread-2: count(%d) time_diff(%ld)\n", count++, diff);
        tx_thread_sleep(TX_MSEC(10000));
        // tx_os_nanosleep(1000000000);
    }
}

void tx_application_define(void *unused) {
	/* Initialize HAL layer */
	HAL_Init();

    tx_thread_spawn(&main_pid, "main_thread", main_thread, &main_pid, main_stack, 
        sizeof(main_stack), MAIN_THREAD_PRIO, MAIN_THREAD_PRIO, TX_NO_TIME_SLICE, TX_AUTO_START);
}
