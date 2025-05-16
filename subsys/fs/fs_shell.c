/*
 * Copyright 2025 wtcat
 */

#include <subsys/fs/fs.h>
#include <subsys/shell/shell.h>

static int fs_shell_list(const struct shell *sh, size_t argc, char *argv[]) {
	if (argc < 2) {
        shell_fprintf(sh, SHELL_NORMAL, "Usage: list <path>\n");
        return -EINVAL;
    }

    struct fs_dir dir = {0};
    int err = fs_opendir(&dir, argv[1]);
    if (err)
        return err;

    struct fs_dirent entry;
    while (fs_readdir(&dir, &entry) == 0) {
        if (entry.type == FS_DIR_ENTRY_FILE) {
            shell_fprintf(sh, SHELL_NORMAL, "<fil>   %-64s     %u bytes\n", 
                entry.name, entry.size);
        } else {
            shell_fprintf(sh, SHELL_NORMAL, "<dir>   %-64s     %u bytes\n", 
                entry.name, entry.size);
        }
    }

    fs_closedir(&dir);
    return 0;
}

static int fs_shell_dump(const struct shell *sh, size_t argc, char *argv[]) {
#define FS_READBUF_SIZE 128
	if (argc < 2) {
        shell_fprintf(sh, SHELL_NORMAL, "Usage: dump <path>\n");
        return -EINVAL;
    }

    struct fs_file fd;
    char buffer[FS_READBUF_SIZE + 1];
    int ret;

    ret = fs_open(&fd, argv[1], FS_O_READ);
    if (ret)
        return ret;

    do {
        ret = fs_read(&fd, buffer, FS_READBUF_SIZE);
        if (ret < FS_READBUF_SIZE) {
            shell_fprintf(sh, SHELL_NORMAL, "\n\n");
            break;
        }
        buffer[ret] = '\0';
        shell_fprintf(sh, SHELL_NORMAL, "%s", buffer);
    } while (true);

    fs_close(&fd);
    return 0;
#undef FS_READBUF_SIZE
}

static int fs_shell_mkdir(const struct shell *sh, size_t argc, char *argv[]) {
	if (argc < 2) {
        shell_fprintf(sh, SHELL_NORMAL, "Usage: mkdir <path>\n");
        return -EINVAL;
    }

    return fs_mkdir(argv[1]);
}

static int fs_shell_remove(const struct shell *sh, size_t argc, char *argv[]) {
	if (argc < 2) {
        shell_fprintf(sh, SHELL_NORMAL, "Usage: remove <path>\n");
        return -EINVAL;
    }

    return fs_unlink(argv[1]);
}

static int fs_shell_rename(const struct shell *sh, size_t argc, char *argv[]) {
	if (argc < 3) {
        shell_fprintf(sh, SHELL_NORMAL, "Usage: rename <from> <to>\n");
        return -EINVAL;
    }

    return fs_rename(argv[1], argv[2]);
}

SHELL_STATIC_SUBCMD_SET_CREATE(fs_cmds, 
	SHELL_CMD_ARG(list,    NULL, "list <path>", fs_shell_list, 2, 0),
    SHELL_CMD_ARG(dump,    NULL, "dump <path>", fs_shell_dump, 2, 0),
    SHELL_CMD_ARG(mkdir,   NULL, "mkdir <path>", fs_shell_mkdir, 2, 0),
    SHELL_CMD_ARG(remove,  NULL, "remove <path>", fs_shell_remove, 2, 0),
    SHELL_CMD_ARG(rename,  NULL, "rename <from> <to>", fs_shell_rename, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(fs, &fs_cmds, "Filesystem operation commands", NULL);
