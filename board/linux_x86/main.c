/*
 * Copyright 2024 wtcat
 */
#include <stdio.h>
#include "tx_api.h"

#include "subsys/fs/fs.h"

#define MAIN_THREAD_PRIO  11
#define MAIN_THREAD_STACK 4096

#define KERNEL_HEAP_SIZE 1024*1024
char _kernel_byte_pool_start[KERNEL_HEAP_SIZE];
UINT _kernel_byte_pool_size = KERNEL_HEAP_SIZE;

static TX_THREAD main_pid;
static ULONG main_stack[MAIN_THREAD_STACK / sizeof(ULONG)];

static void file_test(void);

static void stdio_puts(const char *s, size_t len) {
    fwrite(s, len, 1, stdout);
}

int main(int argc, char *argv[]) {

    __console_puts = stdio_puts;
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

    file_test();

    for ( ; ; ) {
        tx_thread_sleep(TX_MSEC(10000));
    }
}

void tx_application_define(void *unused) {
    tx_thread_spawn(&main_pid, "main_thread", main_thread, &main_pid, main_stack, 
        sizeof(main_stack), MAIN_THREAD_PRIO, MAIN_THREAD_PRIO, TX_NO_TIME_SLICE, TX_AUTO_START);
}


static void file_test(void) {
    int err;

    err = fs_mkfs(FS_EXFATFS, "ramblk", NULL, 0);
    if (err)
        return;

    err = fs_mount("/", "ramblk", FS_EXFATFS, 0, NULL);
    if (err)
        return;

    struct fs_file fd = {0};
    err = fs_open(&fd, "/hello.txt", FS_O_CREATE | FS_O_WRITE);
    if (err)
        goto _unmount;

    fs_write(&fd, "hello world", 11);
    fs_close(&fd);

    struct fs_file rfd = {0};
    char buffer[12] = {0};

    err = fs_open(&rfd, "/hello.txt", FS_O_READ);
    if (err)
        goto _unmount;

    fs_read(&rfd, buffer, 11);
    fs_close(&rfd);

_unmount:
    fs_unmount("/");
    return;
}