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
 * File        cli.h
 * Created by  Sean Farrelly
 * Version     1.0
 *
 */

/*! @file cli.h
 * @brief Command-line interface API definitions.
 */

/*!
 * @defgroup CLI API
 */
#ifndef SUBSYS_CLI_H_
#define SUBSYS_CLI_H_

#include <stddef.h>
#include <stdarg.h>

#include "tx_user.h"
#include "basework/linker.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifndef SUBSYS_CLI_BUF_SIZE
#define SUBSYS_CLI_BUF_SIZE 128
#endif

struct cli_process;
struct cli_command {
	const char *cmd;
    const char *help;
	int (*exec)(struct cli_process *cli, int argc, char *argv[]);
};

struct cli_ifdev {
    void *(*open)(const char *dev);
    int   (*close)(void *dev);
    int   (*getc)(void *dev);
    void  (*puts)(struct cli_process *cli, const char *s, size_t len);
};

struct cli_process {
    const struct cli_ifdev *ifdev;
	const struct cli_command *cmd_tbl;
	size_t cmd_cnt;
    const char *cmd_prompt;
    char cmd_buf[SUBSYS_CLI_BUF_SIZE];
    volatile char *buf_ptr;
    volatile uint8_t cmd_pending;
    void *priv;
};

/* 
 * Define a command 
 */
#define CLI_CMD(_name, _help, _fn) \
    static LINKER_ROSET_ITEM_ORDERED(cli, \
        struct cli_command, _name, _name) = { \
        .cmd  = #_name, \
        .help = _help, \
        .exec = _fn \
    };

int cli_init(struct cli_process *cli);
int cli_deinit(struct cli_process *cli);
int cli_process(struct cli_process *cli);
int cli_input(struct cli_process *cli, char c);
int cli_println(struct cli_process *cli, const char *fmt, ...);

/*
 * os platform api
 */
int cli_run(const char *console, int prio, void *stack, size_t stack_size,
    const char *prompt, const struct cli_ifdev *ifdev);
int cli_stop(void);

/*
 * CLI device list
 */
extern const struct cli_ifdev _cli_ifdev_uart;

#ifdef __cplusplus
}
#endif /* End of CPP guard */
#endif /* SUBSYS_CLI_H_ */
/** @}*/
