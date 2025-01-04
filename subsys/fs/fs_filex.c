/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 *
 */

#include <errno.h>
#include "basework/rte_cpu.h"
#include "tx_api.h"
#include "fx_api.h"

#define FS_PRIVATE_EXTENSION FX_MEDIA media;
#include "subsys/fs/fs.h"

#define FX_ERR(_err)  ((_err)? _FX_ERR(_err): 0)
#define _FX_ERR(_err) -(__ELASTERROR + (int)(_err))

#ifndef FX_MEDIA_BUFFER_SIZE
#define FX_MEDIA_BUFFER_SIZE 512
#endif

static char media_buffer[FX_MEDIA_BUFFER_SIZE] __rte_aligned(RTE_CACHE_LINE_SIZE);

static int filex_fs_open(struct fs_file *fp, const char *file_name, 
    fs_mode_t flags) {
    FX_MEDIA *media = &fp->vfs->media;
    bool created = false;
    UINT fxerr;

    if (flags & FS_O_CREATE) {
        fxerr = fx_file_create(media, (CHAR *)file_name);
        if (fxerr != FX_SUCCESS)
            return FX_ERR(fxerr);
        created = true;
    }

    int rw_flags = flags & (FS_O_WRITE|FS_O_READ);
    if (!rw_flags && !created)
        return -EINVAL;

    FX_FILE *fxp = fp->filep;
    fxerr = fx_file_open(media, fxp, (CHAR *)file_name, 
        rw_flags == FS_O_READ? FX_OPEN_FOR_READ: FX_OPEN_FOR_WRITE);

    return FX_ERR(fxerr);
}

static int filex_fs_close(struct fs_file *fp) {
    FX_FILE *fxp = fp->filep;
    UINT err;

    err = fx_file_close(fxp);
    return FX_ERR(err);
}

static ssize_t filex_fs_read(struct fs_file *fp, void *ptr, size_t size) {
    FX_FILE *fxp = fp->filep;
    ULONG rdbytes;
    UINT err;

    err = fx_file_read(fxp, ptr, size, &rdbytes);
    if (err == FX_SUCCESS)
        return (ssize_t)rdbytes;

    return _FX_ERR(err);
}

static ssize_t filex_fs_write(struct fs_file *fp, const void *ptr, size_t size) {
    FX_FILE *fxp = fp->filep;
    UINT err;

    err = fx_file_write(fxp, (VOID *)ptr, size);
    if (err == FX_SUCCESS)
        return size;

    return _FX_ERR(err);
}

static int filex_fs_lseek(struct fs_file *fp, off_t offset, int whence) {
    FX_FILE *fxp = fp->filep;
    UINT err;

    switch (whence) {
    case FS_SEEK_SET:
        err = fx_file_seek(fxp, offset);
        break;
    case FS_SEEK_CUR:
        if (offset >= 0)
            err = fx_file_relative_seek(fxp, offset, FX_SEEK_FORWARD);
        else
            err = fx_file_relative_seek(fxp, -offset, FX_SEEK_BACK);
        break;
    case FS_SEEK_END:
        err = fx_file_relative_seek(fxp, offset, FX_SEEK_END);
        break;
    }

    return FX_ERR(err);
}

static off_t filex_fs_tell(struct fs_file *fp) {
    FX_FILE *fxp = fp->filep;
    UINT err;
    return -ENOTSUP;
}

static int filex_fs_truncate(struct fs_file *fp, off_t length) {
    FX_FILE *fxp = fp->filep;
    UINT err;

    err = fx_file_truncate(fxp, length);
    return FX_ERR(err);
}

static int filex_fs_sync(struct fs_file *fp) {
    FX_FILE *fxp = fp->filep;
    UINT err;
    return -ENOTSUP;
}

/* Directory operations */
static int filex_fs_opendir(struct fs_dir *dp, const char *abs_path) {
    struct fs_class *fs = dp->vfs;
    FX_FILE *fxp = dp->dirp;

    dp->dirp = (void *)abs_path;
    return 0;
}

static int filex_fs_readdir(struct fs_dir *dp, struct fs_dirent *entry) {
    FX_FILE *fxp = fp->filep;
    UINT err;

    if (dp->dirp == NULL)
        return -EINVAL;

    fx_directory_first_full_entry_find(&fs->media, (CHAR *)abs_path)
    return -ENOTSUP;
}

static int filex_fs_closedir(struct fs_dir *dp) {
    UINT err;
    dp->dirp = NULL;
    return -ENOTSUP;
}

static int filex_fs_mount(struct fs_class *fs) {
    struct device *dev;
    UINT err;

    dev = device_find(fs->storage_dev);
    if (dev == NULL)
        return -ENODEV;

    //TODO: driver handler
    err = fx_media_open(&fs->media, (CHAR *)dev->name, NULL, 
        dev_get_private(dev),
        media_buffer, sizeof(media_buffer));
    
    return FX_ERR(err);
}

static int filex_fs_unmount(struct fs_class *fs) {
    UINT err;

    err = fx_media_close(&fs->media);
    return FX_ERR(err);
}

/* Filesystem operations */
static int filex_fs_mkdir(struct fs_class *fs, const char *abs_path) {
    UINT err;

    err = fx_directory_create(&fs->media, (CHAR *)abs_path);
    return FX_ERR(err);
}

static int filex_fs_unlink(struct fs_class *fs, const char *abs_path) {
    UINT attr, err;

    err = fx_file_attributes_read(&fs->media, (CHAR *)abs_path, &attr);
    if (err == FX_SUCCESS)
        err = fx_file_delete(&fs->media, (CHAR *)abs_path);
    else if (err == FX_NOT_A_FILE)
        err = fx_directory_delete(&fs->media, (CHAR *)abs_path);
    return FX_ERR(err);
}

static int filex_fs_rename(struct fs_class *fs, const char *from, const char *to) {
    UINT attr, err;

    err = fx_file_attributes_read(&fs->media, (CHAR *)from, &attr);
    if (err == FX_SUCCESS)
        err = fx_file_rename(&fs->media, (CHAR *)from, (CHAR *)to);
    else if (err == FX_NOT_A_FILE)
        err = fx_directory_rename(&fs->media, (CHAR *)from, (CHAR *)to);
    return FX_ERR(err);
}

static int filex_fs_stat(struct fs_class *fs, const char *abs_path, struct fs_dirent *entry) {
    UINT err;
    return -ENOTSUP;
}

static int filex_fs_statvfs(struct fs_class *fs, const char *abs_path, 
    struct fs_statvfs *stat) {
    UINT err;
    return -ENOTSUP;
}

static int filex_fs_mkfs(struct fs_class *fs, const char *devname, 
    void *cfg, int flags) {
    struct device *dev;
    UINT err;

    dev = device_find(fs->storage_dev);
    if (dev == NULL)
        return -ENODEV;

    err = fx_media_format(&fs->media,
                    _fx_ram_driver,               // Driver entry
                    dev_get_private(dev),              // RAM disk memory pointer
                    media_buffer,                 // Media buffer pointer
                    sizeof(FX_MEDIA_BUFFER_SIZE),         // Media buffer size
                    "exfat",                // Volume Name
                    1,                            // Number of FATs
                    32,                           // Directory Entries
                    0,                            // Hidden sectors
                    256,                          // Total sectors
                    512,                          // Sector size
                    8,                            // Sectors per cluster
                    1,                            // Heads
                    1);                           // Sectors per track

    return FX_ERR(err);
}


static struct fs_class filex_fs = {
    .fs_ops = {
        .open     = filex_fs_open,
        .read     = filex_fs_read,
        .write    = filex_fs_write,
        .lseek    = filex_fs_lseek,
        .tell     = filex_fs_tell,
        .truncate = filex_fs_truncate,
        .sync     = filex_fs_sync,
        .close    = filex_fs_close,
        .opendir  = filex_fs_opendir,
        .readdir  = filex_fs_readdir,
        .closedir = filex_fs_closedir,
        .mount    = filex_fs_mount,
        .unmount  = filex_fs_unmount,
        .unlink   = filex_fs_unlink,
        .rename   = filex_fs_rename,
        .mkdir    = filex_fs_mkdir,
        .stat     = filex_fs_stat,
        .statvfs  = filex_fs_statvfs,
        .mkfs     = filex_fs_mkfs
    }
};

static int fs_filex_init(void) {
    fx_system_initialize();
    return fs_register(FS_EXFATFS, &filex_fs);
}

SYSINIT(fs_filex_init, SI_FILESYSTEM_LEVEL, 00);
