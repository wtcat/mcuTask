/*
 * Copyright 2025 wtcat
 */

#include <base/linker.h>
#include <subsys/shell/shell.h>

/*
 * Shell section declare
 */
LINKER_ROSET(shell, struct shell);
LINKER_ROSET(shell_root_cmds, union shell_cmd_entry);
LINKER_ROSET(shell_dynamic_subcmds, union shell_cmd_entry);
LINKER_ROSET(shell_subcmds, union shell_cmd_entry);
