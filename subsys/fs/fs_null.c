/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 *
 */

#include <errno.h>
#include "subsys/fs/fs.h"

/* File operations */
static int _fs_null_open(struct fs_file *fp, const char *file_name, fs_mode_t flags) {
    return -ENOTSUP;
}

static int _fs_null_close(struct fs_file *fp) {
    return -ENOTSUP;
}

static ssize_t _fs_null_read(struct fs_file *fp, void *ptr, size_t size) {
    return -ENOTSUP;
}

static ssize_t _fs_null_write(struct fs_file *fp, const void *ptr, size_t size) {
    return -ENOTSUP;
}

static int _fs_null_seek(struct fs_file *fp, off_t offset, int whence) {
    return -ENOTSUP;
}

static off_t _fs_null_tell(struct fs_file *fp) {
    return -ENOTSUP;
}

static int _fs_null_truncate(struct fs_file *fp, off_t length) {
    return -ENOTSUP;
}

static int _fs_null_sync(struct fs_file *fp) {
    return -ENOTSUP;
}

/* Directory operations */
static int _fs_null_opendir(struct fs_dir *dp, const char *abs_path) {
    return -ENOTSUP;
}

static int _fs_null_readdir(struct fs_dir *dp, struct fs_dirent *entry) {
    return -ENOTSUP;
}

static int _fs_null_closedir(struct fs_dir *dp) {
    return -ENOTSUP;
}

/* Filesystem operations */
static int _fs_null_mkdir(struct fs_class *fs, const char *abs_path) {
    return -ENOTSUP;
}

static int _fs_null_unlink(struct fs_class *fs, const char *abs_path) {
    return -ENOTSUP;
}

static int _fs_null_rename(struct fs_class *fs, const char *from, const char *to) {
    return -ENOTSUP;
}

static int _fs_null_stat(struct fs_class *fs, const char *abs_path, 
    struct fs_stat *entry) {
    return -ENOTSUP;
}

static int _fs_null_statvfs(struct fs_class *fs, const char *abs_path, 
    struct fs_statvfs *stat) {
    return -ENOTSUP;
}

static int _fs_null_mount(struct fs_class *fs) {
    return -ENOTSUP;
}

static int _fs_null_unmount(struct fs_class *fs) {
    return -ENOTSUP;
}

static int _fs_null_mkfs(const char *devname, void *cfg, int flags) {
    return -ENOTSUP;
}

const struct fs_operations _fs_default_operation = {
    .open     = _fs_null_open,
    .read     = _fs_null_read,
    .write    = _fs_null_write,
    .lseek    = _fs_null_seek,
    .tell     = _fs_null_tell,
    .truncate = _fs_null_truncate,
    .sync     = _fs_null_sync,
    .close    = _fs_null_close,
    .opendir  = _fs_null_opendir,
    .readdir  = _fs_null_readdir,
    .closedir = _fs_null_closedir,
    .mount    = _fs_null_mount,
    .unmount  = _fs_null_unmount,
    .unlink   = _fs_null_unlink,
    .rename   = _fs_null_rename,
    .mkdir    = _fs_null_mkdir,
    .stat     = _fs_null_stat,
    .statvfs  = _fs_null_statvfs,
    .mkfs     = _fs_null_mkfs
};
