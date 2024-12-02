/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 */

#include <string.h>
#include "subsys/cli/cli.h"


static bool cmd_iterator(const struct cli_command *cmd, void *arg) {
    struct cli_process *cli = arg;

    cli_println(cli, "  %-30s  %s\n",
        cmd->usage, cmd->help);
    return false;
}

static int cli_cmd_help(struct cli_process *cli, int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    cli_println(cli, "\nAll commands list:\n\n");
    cli_foreach(cli, cmd_iterator, cli);
	return 0;
}

CLI_CMD(help, "help",
    "List all commands",
    cli_cmd_help
)
