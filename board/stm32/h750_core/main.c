/*
 * Copyright 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE
#define TX_USE_SECTION_INIT_API_EXTENSION 1

#include "tx_api.h"
#include "fx_api.h"

#include "basework/rte_cpu.h"
#include "basework/os/osapi.h"
#include "basework/log.h"

#include "subsys/cli/cli.h"
#include "subsys/fs/fs.h"

#define MAIN_THREAD_PRIO  11
#define MAIN_THREAD_STACK 4096

static TX_THREAD main_pid;
static ULONG main_stack[MAIN_THREAD_STACK / sizeof(ULONG)] __rte_section(".dtcm");
static struct fs_class main_fs = {
    .mnt_point = "/home",
    .type = FS_EXFATFS,
    .storage_dev = "mmcblk0"
};

static int file_dump(const char *filename) {
    struct fs_file fd = {0};
    char buffer[257];
    int err;

    err = fs_open(&fd, filename, FS_O_READ);
    if (err)
        return err;
    
    do {
        int ret = fs_read(&fd, buffer, sizeof(buffer) - 1);
        if (ret < 0)
            break;

        buffer[ret] = '\0';
        printk("%s", buffer);
    } while (1);

    fs_close(&fd);
    return 0;
}

static void __rte_unused file_test(void) {
    int err;
    char path[128] = {"/home/X"};

    err = fs_mkdir("/home/a");
    if (err)
        goto _unmount;

    err = fs_mkdir("/home/b");
    if (err)
        goto _unmount;

    struct fs_file fd = {0};
    err = fs_open(&fd, "/home/hello.txt", FS_O_CREATE | FS_O_WRITE);
    if (err)
        goto _unmount;
    fs_write(&fd, "hello world", 11);
    fs_close(&fd);

    struct fs_file rfd = {0};
    char buffer[12] = {0};
    err = fs_open(&rfd, "/home/hello.txt", FS_O_READ);
    if (err)
        goto _unmount;

    fs_seek(&rfd, 0, FS_SEEK_END);
    size_t fsize = fs_tell(&rfd);
    pr_out("file size: %d\n", fsize);
    fs_seek(&rfd, 0, FS_SEEK_SET);

    struct fs_stat stat;
    fs_stat("/home/hello.txt", &stat);
    if ((size_t)stat.st_size != fsize)
        pr_out("fs_stat failed to get file size\n");
    
    fs_read(&rfd, buffer, fsize);
    fs_close(&rfd);
    if (strcmp(buffer, "hello world"))
        pr_out("failed to read file\n");

    struct fs_dir dir = {0};
    err = fs_opendir(&dir, "/home/");
    if (err)
        goto _unmount;
    
    struct fs_dirent entry;
    while ((err = fs_readdir(&dir, &entry)) == 0) {
        pr_out("dir: %s size: %d\n", entry.name, entry.size);
    }
    fs_closedir(&dir);


    err = fs_opendir(&dir, "/home/");
    if (err)
        goto _unmount;
    
    while ((err = fs_readdir(&dir, &entry)) == 0) {
        pr_out("new-dir: %s size: %d\n", entry.name, entry.size);
        if (entry.type == FS_DIR_ENTRY_FILE) {
            strlcpy(path + 6, entry.name, 120);
            file_dump(path);
        }
    }
    fs_closedir(&dir);
    fs_flush("/home");

_unmount:
    err = fs_unmount("/home");
    pr_out("Unmount /home with error(%d)\n", err);
    return;
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

    tx_thread_sleep(TX_MSEC(300));
    
    /* Mount file system */
    if (!fs_mount(&main_fs))
        printk("Mount filesystem success\n");
    
    /*
     * Create command line interface
     */
    static ULONG stack[1024];
    cli_run("uart1", 15, stack, sizeof(stack), 
        "[task]# ", &_cli_ifdev_uart);

    file_test();

    extern int main(void);
    main();
}

void tx_application_define(void *unused) {
	/* Initialize HAL layer */
	HAL_Init();

    tx_thread_spawn(&main_pid, "main_thread", main_thread, &main_pid, main_stack, 
        sizeof(main_stack), MAIN_THREAD_PRIO, MAIN_THREAD_PRIO, TX_NO_TIME_SLICE, TX_AUTO_START);
}
