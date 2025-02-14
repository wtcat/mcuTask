/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 */

#include <errno.h>
#include <string.h>

#include "fx_api.h"

#include "subsys/fs/fs.h"
#include "subsys/cli/cli.h"


static int cli_cmd_mkfat(struct cli_process *cli, int argc, char *argv[]) {
    if (argc != 2)
        return -EINVAL;

    return fs_mkfs(FS_EXFATFS, argv[1], NULL, 0);
}
CLI_CMD(mkfat, "mkfat devname",
    "Format exFAT filesystem",
    cli_cmd_mkfat
)

static int cli_cmd_mountfat(struct cli_process *cli, int argc, char *argv[]) {
    if (argc != 2)
        return -EINVAL;

    static FX_MEDIA fs_media;
    static struct fs_class main_fs = {
        .mnt_point = "/c",
        .type = FS_EXFATFS,
        .storage_dev = "sdblk0",
        .fs_data = &fs_media
    };
    return fs_mount(&main_fs);
}
CLI_CMD(mount, "mount devname",
    "Mount exFAT filesystem",
    cli_cmd_mountfat
)
