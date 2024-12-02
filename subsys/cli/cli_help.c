/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 */

#include <string.h>
#include "subsys/cli/cli.h"

LINKER_ROSET(cli, struct cli_command);

static void 
cli_show_cmd(struct cli_process *cli, const struct cli_command *cmd) {
    size_t len = strlen(cmd->usage);
    cli_println(cli, "\t%s -- %-*.*s\n",
        cmd->usage, len, cmd->help);
}

static int cli_cmd_help(struct cli_process *cli, int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    cli_println(cli, "\nAll commands list:\n\n");

    LINKER_SET_FOREACH(cli, item, struct cli_command) {
        cli_show_cmd(cli, (const struct cli_command *)item);
    }
    
	for (size_t i = 0; i < cli->cmd_cnt; i++)
        cli_show_cmd(cli, &cli->cmd_tbl[i]);

	return 0;
}

CLI_CMD(help, "help",
    "List all commands",
    cli_cmd_help
)
