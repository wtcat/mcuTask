/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 *
 */

#include <errno.h>
#include "subsys/fs/fs.h"

/* File operations */
int _fs_null_open(struct fs_file *fp, const char *file_name, fs_mode_t flags) {
    return -ENOTSUP;
}

int _fs_null_close(struct fs_file *fp) {
    return -ENOTSUP;
}

ssize_t _fs_null_read(struct fs_file *fp, void *ptr, size_t size) {
    return -ENOTSUP;
}

ssize_t _fs_null_write(struct fs_file *fp, const void *ptr, size_t size) {
    return -ENOTSUP;
}

int _fs_null_seek(struct fs_file *fp, off_t offset, int whence) {
    return -ENOTSUP;
}

off_t _fs_null_tell(struct fs_file *fp) {
    return -ENOTSUP;
}

int _fs_null_truncate(struct fs_file *fp, off_t length) {
    return -ENOTSUP;
}

int _fs_null_sync(struct fs_file *fp) {
    return -ENOTSUP;
}

/* Directory operations */
int _fs_null_opendir(struct fs_dir *dp, const char *abs_path) {
    return -ENOTSUP;
}

int _fs_null_readdir(struct fs_dir *dp, struct fs_dirent *entry) {
    return -ENOTSUP;
}

int _fs_null_closedir(struct fs_dir *dp) {
    return -ENOTSUP;
}

/* Filesystem operations */
int _fs_null_mkdir(const char *abs_path) {
    return -ENOTSUP;
}

int _fs_null_unlink(const char *abs_path) {
    return -ENOTSUP;
}

int _fs_null_rename(const char *from, const char *to) {
    return -ENOTSUP;
}

int _fs_null_stat(const char *abs_path, struct fs_dirent *entry) {
    return -ENOTSUP;
}

int _fs_null_statvfs(const char *abs_path, struct fs_statvfs *stat) {
    return -ENOTSUP;
}
