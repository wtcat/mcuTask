/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 */

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>

#include <subsys/shell/shell.h>

#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)
#define MAX_MEM_SIZE (32 * 1024 * 1024)
#define DISP_LINE_LEN 16

static void 
format_buffer(const struct shell *sh, const char *addr, int width, 
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

		shell_fprintf(sh, SHELL_NORMAL, "%08x:", (unsigned int)disp_addr);

		/* check for overflow condition */
		if (count < thislinelen)
			thislinelen = count;

		/* Copy from memory into linebuf and print hex values */
		for (i = 0; i < thislinelen; i++) {
			if (width == 4) {
				lb.ui[i] = *(volatile uint32_t *)data;
				shell_fprintf(sh, SHELL_NORMAL, " %08"PRIx32, lb.ui[i]);
			} else if (width == 2) {
				lb.us[i] = *(volatile uint16_t *)data;
				shell_fprintf(sh, SHELL_NORMAL, " %04"PRIx16, lb.us[i]);
			} else {
				lb.uc[i] = *(volatile uint8_t *)data;
				shell_fprintf(sh, SHELL_NORMAL, " %02"PRIx8, lb.uc[i]);
			}
			data += width;
		}

		while (thislinelen < linelen) {
			/* fill line with whitespace for nice ASCII print */
			for (i = 0; i < width * 2 + 1; i++)
				shell_fprintf(sh, SHELL_NORMAL, " ");
			linelen--;
		}

		/* Print data in ASCII characters */
		for (i = 0; i < thislinelen * width; i++) {
			if (lb.uc[i] < 0x20 || lb.uc[i] > 0x7e)
				lb.uc[i] = '.';
		}
		lb.uc[i] = '\0';
		shell_fprintf(sh, SHELL_NORMAL, "    %s\n", lb.uc);

		/* update references */
		addr += thislinelen * width;
		disp_addr += thislinelen * width;
		count -= thislinelen;
	}
}

static int 
do_mem_mw(const struct shell *sh, int width, size_t argc, char *argv[]) {
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
			shell_fprintf(sh, SHELL_NORMAL, "params invalid.\n");
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
do_mem_md(const struct shell *sh, int width, size_t argc, char *argv[]) {
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
			shell_fprintf(sh, SHELL_NORMAL, "params invalid.\n");
			count = 0;
			return -EINVAL;
		}
	} else
		count = 1;

	if (count != ULONG_MAX) {
		format_buffer(sh, (char *)addr, width, count, 
            DISP_LINE_LEN / width, -1);
    }

	return 0;
}

static int cli_cmd_mdw(const struct shell *sh, size_t argc, char *argv[]) {
	return do_mem_md(sh, 4, argc, argv);
}

static int cli_cmd_mdh(const struct shell *sh, size_t argc, char *argv[]) {
	return do_mem_md(sh, 2, argc, argv);
}

static int cli_cmd_mdb(const struct shell *sh, size_t argc, char *argv[]) {
	return do_mem_md(sh, 1, argc, argv);
}

static int cli_cmd_mww(const struct shell *sh, size_t argc, char *argv[]) {
	return do_mem_mw(sh, 4, argc, argv);
}

static int cli_cmd_mwh(const struct shell *sh, size_t argc, char *argv[]) {
	return do_mem_mw(sh, 2, argc, argv);
}

static int cli_cmd_mwb(const struct shell *sh, size_t argc, char *argv[]) {
	return do_mem_mw(sh, 1, argc, argv);
}


SHELL_STATIC_SUBCMD_SET_CREATE(memory_cmds, 
	SHELL_CMD_ARG(mdw, NULL, "mdw <address> [count]", cli_cmd_mdw, 2, 1),
	SHELL_CMD_ARG(mdh, NULL, "mdh <address> [count]", cli_cmd_mdh, 2, 1),
	SHELL_CMD_ARG(mdb, NULL, "mdb <address> [count]", cli_cmd_mdb, 2, 1),

	SHELL_CMD_ARG(mww, NULL, "mww <address> <value> [count]", cli_cmd_mww, 3, 1),
	SHELL_CMD_ARG(mwh, NULL, "mwh <address> <value> [count]", cli_cmd_mwh, 3, 1),
	SHELL_CMD_ARG(mwb, NULL, "mwb <address> <value> [count]", cli_cmd_mwb, 3, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(mem, &memory_cmds, "Memory read/write commands", NULL);
