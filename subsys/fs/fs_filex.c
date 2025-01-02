/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 *
 */

#include <errno.h>
#include "fx_api.h"

#define FS_PRIVATE_EXTENSION FX_MEDIA media;
#include "subsys/fs/fs.h"


#define FX_OPEN_FOR_READ                       0
#define FX_OPEN_FOR_WRITE                      1
#define FX_OPEN_FOR_READ_FAST                  2

static int filex_fs_open(struct fs_file *fp, const char *file_name, fs_mode_t flags) {
    FX_MEDIA *media = &fp->vfs->media;
    UINT fxerr;

    if (flags & FS_O_CREATE) {
        fxerr = fx_file_create(media, (CHAR *)file_name);
        if (fxerr != FX_SUCCESS)
            return fxerr;
    }

    fs_mode_t rw_flags = flags & (FS_O_WRITE|FS_O_READ);
    if (rw_flags) {
        FX_FILE *fxp = fp->filep;
        fxerr = fx_file_open(media, fxp, (CHAR *)file_name, 
            rw_flags == FS_O_READ? FX_OPEN_FOR_READ: FX_OPEN_FOR_WRITE);

    }

    return -ENOTSUP;
}

static int filex_fs_close(struct fs_file *fp) {
    return -ENOTSUP;
}

static ssize_t filex_fs_read(struct fs_file *fp, void *ptr, size_t size) {
    return -ENOTSUP;
}

static ssize_t filex_fs_write(struct fs_file *fp, const void *ptr, size_t size) {
    return -ENOTSUP;
}

static int filex_fs_seek(struct fs_file *fp, off_t offset, int whence) {
    return -ENOTSUP;
}

static off_t filex_fs_tell(struct fs_file *fp) {
    return -ENOTSUP;
}

static int filex_fs_truncate(struct fs_file *fp, off_t length) {
    return -ENOTSUP;
}

static int filex_fs_sync(struct fs_file *fp) {
    return -ENOTSUP;
}

/* Directory operations */
static int filex_fs_opendir(struct fs_dir *dp, const char *abs_path) {
    return -ENOTSUP;
}

static int filex_fs_readdir(struct fs_dir *dp, struct fs_dirent *entry) {
    return -ENOTSUP;
}

static int filex_fs_closedir(struct fs_dir *dp) {
    return -ENOTSUP;
}

/* Filesystem operations */
static int filex_fs_mkdir(const char *abs_path) {
    return -ENOTSUP;
}

static int filex_fs_unlink(const char *abs_path) {
    return -ENOTSUP;
}

static int filex_fs_rename(const char *from, const char *to) {
    return -ENOTSUP;
}

static int filex_fs_stat(const char *abs_path, struct fs_dirent *entry) {
    return -ENOTSUP;
}

static int filex_fs_statvfs(const char *abs_path, struct fs_statvfs *stat) {
    return -ENOTSUP;
}


static struct fs_class filex_fs = {
    .fs_ops = {
        
    }
};
