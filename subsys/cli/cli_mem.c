/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "subsys/cli/cli.h"

#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)
#define MAX_MEM_SIZE (32 * 1024 * 1024)
#define DISP_LINE_LEN 16

static void 
format_buffer(struct cli_process *cli, const char *addr, int width, 
    int count, int linelen, unsigned long disp_addr) {
	int i, thislinelen;
	const char *data;

	/* linebuf as a union causes proper alignment */
	union linebuf {
		uint32_t ui[MAX_LINE_LENGTH_BYTES / sizeof(uint32_t) + 1];
		uint16_t us[MAX_LINE_LENGTH_BYTES / sizeof(uint16_t) + 1];
		uint8_t uc[MAX_LINE_LENGTH_BYTES / sizeof(uint8_t) + 1];
	} lb;

	if (linelen * width > MAX_LINE_LENGTH_BYTES)
		linelen = MAX_LINE_LENGTH_BYTES / width;
	if (linelen < 1)
		linelen = DEFAULT_LINE_LENGTH_BYTES / width;

	if (disp_addr == (unsigned long)-1)
		disp_addr = (unsigned long)addr;

	while (count) {
		thislinelen = linelen;
		data = (const char *)addr;

		cli_println(cli, "%08x:", (unsigned int)disp_addr);

		/* check for overflow condition */
		if (count < thislinelen)
			thislinelen = count;

		/* Copy from memory into linebuf and print hex values */
		for (i = 0; i < thislinelen; i++) {
			if (width == 4) {
				lb.ui[i] = *(volatile uint32_t *)data;
				cli_println(cli, " %08x", lb.ui[i]);
			} else if (width == 2) {
				lb.us[i] = *(volatile uint16_t *)data;
				cli_println(cli, " %04x", lb.us[i]);
			} else {
				lb.uc[i] = *(volatile uint8_t *)data;
				cli_println(cli, " %02x", lb.uc[i]);
			}
			data += width;
		}

		while (thislinelen < linelen) {
			/* fill line with whitespace for nice ASCII print */
			for (i = 0; i < width * 2 + 1; i++)
				cli_println(cli, " ");
			linelen--;
		}

		/* Print data in ASCII characters */
		for (i = 0; i < thislinelen * width; i++) {
			if (lb.uc[i] < 0x20 || lb.uc[i] > 0x7e)
				lb.uc[i] = '.';
		}
		lb.uc[i] = '\0';
		cli_println(cli, "    %s\n", lb.uc);

		/* update references */
		addr += thislinelen * width;
		disp_addr += thislinelen * width;
		count -= thislinelen;
	}
}

static int 
do_mem_mw(struct cli_process *cli, int width, int argc, char *argv[]) {
	unsigned long writeval;
	unsigned long addr, count;
	char *buf;
	char *pend;

	if (argc < 3)
		return -EINVAL;

	addr = strtoul(argv[1], NULL, 16);
	writeval = strtoul(argv[2], NULL, 16);
	if (argc == 4) {
		errno = 0;
		count = strtoul(argv[3], &pend, 16);
		if ((pend == argv[3] || *pend != '\0') || errno == ERANGE ||
			count > MAX_MEM_SIZE) {
			cli_println(cli, "params invalid.\n");
			return -EINVAL;
		}
	} else
		count = 1;

	buf = (char *)addr;
	if (count != ULONG_MAX) {
		if (count > 0) {
			while (count-- > 0) {
				if (width == 4)
					*((uint32_t *)buf) = (uint32_t)writeval;
				else if (width == 2)
					*((uint16_t *)buf) = (uint16_t)writeval;
				else
					*((uint8_t *)buf) = (uint8_t)writeval;
				buf += width;
			}
		}
	}
	return 0;
}

static int 
do_mem_md(struct cli_process *cli, int width, int argc, char *argv[]) {
	unsigned long addr;
	unsigned long count;
	char *pend;

	if (argc < 2)
		return -EINVAL;

	addr = strtoul(argv[1], NULL, 16);
	if (argc == 3) {
        errno = 0;
		count = strtoul(argv[2], &pend, 16);
		if ((pend == argv[2] || *pend != '\0') || errno == ERANGE ||
			count > MAX_MEM_SIZE) {
			cli_println(cli, "params invalid.\n");
			count = 0;
			return -EINVAL;
		}
	} else
		count = 1;

	if (count != ULONG_MAX) {
		format_buffer(cli, (char *)addr, width, count, 
            DISP_LINE_LEN / width, -1);
    }

	return 0;
}

static int cli_cmd_mdw(struct cli_process *cli, int argc, char *argv[]) {
	return do_mem_md(cli, 4, argc, argv);
}
CLI_CMD(mdw, "mdw address [count]",
    "Show memory by word",
    cli_cmd_mdw
)

static int cli_cmd_mdh(struct cli_process *cli, int argc, char *argv[]) {
	return do_mem_md(cli, 2, argc, argv);
}
CLI_CMD(mdh, "mdh address [count]",
    "Show memory by half-word",
    cli_cmd_mdh
)

static int cli_cmd_mdb(struct cli_process *cli, int argc, char *argv[]) {
	return do_mem_md(cli, 1, argc, argv);
}
CLI_CMD(mdb, "mdb address [count]",
    "Show memory by byte",
    cli_cmd_mdb
)

static int cli_cmd_mww(struct cli_process *cli, int argc, char *argv[]) {
	return do_mem_mw(cli, 4, argc, argv);
}
CLI_CMD(mww, "mww address [,count]",
    "Write memory by word",
    cli_cmd_mww
)

static int cli_cmd_mwh(struct cli_process *cli, int argc, char *argv[]) {
	return do_mem_mw(cli, 2, argc, argv);
}
CLI_CMD(mwh, "mwh address [,count]",
    "Write memory by half-word",
    cli_cmd_mwh
)

static int cli_cmd_mwb(struct cli_process *cli, int argc, char *argv[]) {
	return do_mem_mw(cli, 1, argc, argv);
}
CLI_CMD(mwb, "mwb address [,count]",
    "Write memory by byte",
    cli_cmd_mwb
)
