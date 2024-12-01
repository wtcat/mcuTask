/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 */

#include <errno.h>

#include "tx_api.h"
#include "subsys/cli/cli.h"

static struct cli_process cli_inst;
static TX_THREAD cli_pid;
static TX_MUTEX cli_mtx;

static void cli_thread(void *arg) {
    struct cli_process *cli = (struct cli_process *)arg;
    const struct cli_ifdev *ifdev = cli->ifdev;

    for ( ; ; ) {
        int c = ifdev->getc(cli->priv);
        if (c < 0)
            continue;
        
        if (cli_input(cli, (char)c) < 0)
            continue;

        cli_process(cli);
    }
}

int cli_run(const char *console, int prio, void *stack, size_t stack_size,
    const char *prompt, const struct cli_ifdev *ifdev) {
    struct cli_process *cli = &cli_inst;
    int err;

    if (console == NULL || ifdev == NULL)
        return -EINVAL;

    guard(os_mutex)(&cli_mtx);
    if (cli->priv)
        return -EBUSY;

    cli->priv = ifdev->open(console);
    if (!cli->priv)
        return -ENODEV;

    cli->cmd_prompt = prompt;
    cli->ifdev = ifdev;
    err = cli_init(cli);
    if (err < 0)
        goto _close;
    
    tx_thread_spawn(&cli_pid, "cli", cli_thread, cli, stack, 
        stack_size, prio, prio, TX_NO_TIME_SLICE, TX_AUTO_START);
    return 0;

_close:
    if (ifdev->close)
        ifdev->close(cli->priv);
    return err;
}

int cli_stop(void) {
    struct cli_process *cli = &cli_inst;

    guard(os_mutex)(&cli_mtx);
    if (cli->priv == NULL)
        return -EACCES;

    tx_thread_terminate(&cli_pid);
    tx_thread_delete(&cli_pid);
    if (cli->ifdev)
        cli->ifdev->close(cli->priv);
    cli_deinit(cli);

    return 0;
}

static int _cli_init(void) {
    tx_mutex_create(&cli_mtx, "cli", TX_INHERIT);
    return 0;
}

SYSINIT(_cli_init, SI_PREDRIVER_LEVEL, 80);
