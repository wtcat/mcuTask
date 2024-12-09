/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 */

#include "tx_api.h"
#include "subsys/cli/cli.h"

static void *cli_uart_open(const char *dev) {
    struct device *udev;

    if (!uart_open(dev, &udev))
        return udev;
    return NULL;
}

static int cli_uart_close(void *dev) {
    return uart_close(dev);
}

static int cli_uart_getc(void *dev) {
    char c;
    if (uart_read(dev, &c, 1, 0) > 0)
        return c;
    return -1;
}

static void cli_uart_puts(struct cli_process *cli, const char *s, size_t len) {
    uart_write(cli->priv, s, len, 0);
}

const struct cli_ifdev _cli_ifdev_uart = {
    .open  = cli_uart_open,
    .close = cli_uart_close,
    .getc  = cli_uart_getc,
    .puts  = cli_uart_puts
};
