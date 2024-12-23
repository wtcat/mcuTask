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
#include "basework/lib/iovpr.h"
#include "subsys/cli/cli.h"

struct find_arg {
	const struct cli_command *cmd;
	const char *name;
};

struct format_buffer {
    struct cli_process *cli;
    char   buf[128];
    size_t len;
};

LINKER_ROSET(cli, struct cli_command);
static const char cli_prompt[] = ">> ";
static const char cli_unrecog[] = "Command not found\r\n";

static inline void 
cli_puts(struct cli_process *cli, const char *msg) {
	cli->ifdev->puts(cli, msg, strlen(msg));
}

static void cli_fputc(int c, void *arg) {
    struct format_buffer *p = (struct format_buffer *)arg;

    if (p->len < sizeof(p->buf) - 2) {
        if (c == '\n')
            p->buf[p->len++] = '\r';
		p->buf[p->len++] = (char)c;
    } else {
        p->buf[p->len] = '\0';
		p->cli->ifdev->puts(p->cli, p->buf, p->len);
        p->len = 0;
    }
}

static bool
cli_find_cb(const struct cli_command *cmd, void *arg) {
	struct find_arg *p = (struct find_arg *)arg;
	rte_mb();
	if (!strcmp(cmd->cmd, p->name)) {
		p->cmd = cmd;
		return true;
	}
	return false;
}

static const struct cli_command *
cli_find(struct cli_process *cli, const char *cmd) {
	struct find_arg param = {NULL, cmd};
	cli_foreach(cli, cli_find_cb, &param);
	return param.cmd;
}

int cli_init(struct cli_process *cli) {
	if (!cli->cmd_prompt)
		cli->cmd_prompt = cli_prompt;
	cli->cmd_pending = 0;
	cli->buf_ptr = cli->cmd_buf;
	cli_puts(cli, cli->cmd_prompt);
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

	if (argc > 0) {
		const struct cli_command *clic = cli_find(cli, argv[0]);
		if (clic != NULL) {
			int ret = clic->exec(cli, argc, argv);

			/* Print the CLI prompt to the user. */
			cli_puts(cli, cli->cmd_prompt); 
			cli->cmd_pending = 0;
			return ret;
		}

		/* Command not found */
		char c = argv[0][0];
		if (c != '\n' && c != '\r')
			cli_puts(cli, cli_unrecog);
	}
	
	/* Print the CLI prompt to the user. */
	cli_puts(cli, cli->cmd_prompt);
	cli->cmd_pending = 0;

	return -ENODATA;
}

int cli_input(struct cli_process *cli, char c) {
	char   echo_buf[4];
	size_t len;

	if (cli->cmd_pending)
		return -EBUSY;

	len = 0;
	switch (c) {
	case '\n':
		break;
	case '\r':
		if (!cli->cmd_pending) {
			*cli->buf_ptr = '\0'; /* Terminate the msg and reset the msg ptr. */
			cli->cmd_pending = 1;
			cli->buf_ptr = cli->cmd_buf; /* Reset buf_ptr to beginning. */
			echo_buf[len++] = '\r';
			echo_buf[len++] = '\n';
		}
		break;
	case '\b':
		/* Backspace. Delete character. */
		if (cli->buf_ptr > cli->cmd_buf) {
			cli->buf_ptr--;
			echo_buf[len++] = '\b';
			echo_buf[len++] = ' ';
			echo_buf[len++] = '\b';
		}
		break;
	default:
		/* Normal character received, add to buffer. */
		if ((cli->buf_ptr - cli->cmd_buf) < SUBSYS_CLI_BUF_SIZE) {
			*cli->buf_ptr++ = c;
			echo_buf[len++] = c;
			break;
		}
		return -E2BIG;
	}

	/* Echo char */
	cli->ifdev->puts(cli, echo_buf, len);

	return 0;
}

int cli_println(struct cli_process *cli, const char *fmt, ...) {
    struct format_buffer fb;
	va_list ap;
    int len;

    fb.cli = cli;
    fb.len = 0;

	va_start(ap, fmt);
    len = _IO_Vprintf(cli_fputc, &fb, fmt, ap);
	va_end(ap);
    if (fb.len > 0) {
        fb.buf[fb.len] = '\0';
		cli->ifdev->puts(cli, fb.buf, fb.len);
    }

    return len;
}

void cli_foreach(struct cli_process *cli, 
	bool (*iter)(const struct cli_command *, void *), 
	void *arg) {
	if (cli == NULL || iter == NULL)
		return;

    LINKER_SET_FOREACH(cli, item, struct cli_command) {
        if (iter((const struct cli_command *)item, arg))
			return;
    }

	for (size_t i = 0; i < cli->cmd_cnt; i++) {
		if (iter(&cli->cmd_tbl[i], arg))
			return;
	}
}
