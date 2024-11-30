/*
 * MIT License
 *
 * Copyright (c) 2019 Sean Farrelly
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * File        cli.c
 * Created by  Sean Farrelly
 * Version     1.0
 *
 *
 * Copyright (c) 2024 wtcat (wt1454246140@gmail.com)
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "basework/rte_atomic.h"
#include "subsys/cli/cli.h"


LINKER_ROSET(cli, struct cli_command);
static const char cli_prompt[] = ">> "; /* CLI prompt displayed to the user */
static const char cli_unrecog[] = "CMD: Command not recognised\r\n";

static inline void 
cli_print(struct cli_process *cli, const char *msg) {
	cli->println(cli, msg);
}

static const struct cli_command *
cli_find(struct cli_process *cli, const char *cmd) {
    LINKER_SET_FOREACH(cli, item, struct cli_command) {
        if (!strcmp(cmd, item->cmd))
			return (const struct cli_command *)item;
    }
	for (size_t i = 0; i < cli->cmd_cnt; i++) {
		if (!strcmp(cmd, cli->cmd_tbl[i].cmd))
			return &cli->cmd_tbl[i];
	}
	return NULL;
}

int cli_init(struct cli_process *cli) {
	if (!cli->cmd_prompt)
		cli->cmd_prompt = cli_prompt;
	cli->cmd_pending = 0;
	cli_print(cli, cli->cmd_prompt);
	return 0;
}

int cli_deinit(struct cli_process *cli) {
	(void)cli;
	return 0;
}

int cli_process(struct cli_process *cli) {
	if (!cli->cmd_pending)
		return 0;

	uint8_t argc = 0;
	char *argv[30];

	/* Get the first token (cmd name) */
	argv[argc] = strtok(cli->cmd_buf, " ");
	while ((argv[argc] != NULL) && (argc < 30))
		argv[++argc] = strtok(NULL, " ");

	const struct cli_command *clic = cli_find(cli, argv[0]);
	if (clic != NULL) {
		int ret = clic->exec(argc, argv);

		/* Print the CLI prompt to the user. */
		cli_print(cli, cli_prompt); 
		cli->cmd_pending = 0;
		return ret;
	}
	
	/* Command not found */
	cli_print(cli, cli_unrecog);
	cli_print(cli, cli_prompt); /* Print the CLI prompt to the user. */

	cli->cmd_pending = 0;
	return -ENODATA;
}

int cli_input(struct cli_process *cli, char c) {
	if (cli->cmd_pending)
		return -EBUSY;

	switch (c) {
	case '\r':
		if (!cli->cmd_pending) {
			*cli->buf_ptr = '\0'; /* Terminate the msg and reset the msg ptr. */
			cli->cmd_pending = 1;
            rte_wmb();
			cli->buf_ptr = cli->cmd_buf; /* Reset buf_ptr to beginning. */
		}
		break;
	case '\b':
		/* Backspace. Delete character. */
		if (cli->buf_ptr > cli->cmd_buf)
			cli->buf_ptr--;
		break;
	default:
		/* Normal character received, add to buffer. */
		if ((cli->buf_ptr - cli->cmd_buf) < SUBSYS_CLI_BUF_SIZE)
			*cli->buf_ptr++ = c;
		else
			return -E2BIG;
		break;
	}
	return 0;
}
