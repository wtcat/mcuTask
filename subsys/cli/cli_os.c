/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 */

#include <errno.h>

#include "tx_api.h"
#include "subsys/cli/cli.h"


static TX_THREAD_DEFINE(cli_pid, 1024)
static struct cli_process cli_inst;
static TX_MUTEX cli_mtx;

static void 
cli_println(struct cli_process *cli, const char *s) {
    uart_write(cli->priv, s, strlen(s), 0);
}

static void cli_thread(void *arg) {
    struct cli_process *cli = arg;
    char c;

    for ( ; ; ) {
        int ret = uart_read(cli->priv, &c, 1, 0);
        if (ret < 0)
            continue;
        
        ret = cli_input(cli, c);
        if (ret < 0)
            continue;

        cli_process(cli);
    }
}

int cli_run(const char *console, int prio) {
    struct cli_process *cli = &cli_inst;
    int err;

    if (console == NULL)
        return -EINVAL;

    guard(os_mutex)(&cli_mtx);
    if (cli->priv)
        return -EBUSY;

    err = uart_open(console, &cli->priv);
    if (err < 0)
        return err;

    cli->println = cli_println;
    err = cli_init(cli);
    if (err < 0)
        goto _close;

    tx_thread_spawn(&cli_pid.task, "cli", cli_thread, cli, cli_pid.stack, 
        sizeof(cli_pid.stack), prio, prio, TX_NO_TIME_SLICE, TX_AUTO_START);
    return 0;

_close:
    uart_close(cli->priv);
    return err;
}

int cli_stop(void) {
    struct cli_process *cli = &cli_inst;

    guard(os_mutex)(&cli_mtx);
    if (cli->priv == NULL)
        return -EACCES;

    tx_thread_terminate(&cli_pid.task);
    tx_thread_delete(&cli_pid.task);
    uart_close(cli->priv);
    cli_deinit(cli);

    return 0;
}

static int _cli_init(void) {
    tx_mutex_create(&cli_mtx, "cli", TX_INHERIT);
    return 0;
}

SYSINIT(_cli_init, SI_PREDRIVER_LEVEL, 80);
