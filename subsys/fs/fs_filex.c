/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 *
 */

#define pr_fmt(fmt) "[filex]: "fmt

#include <errno.h>
#include <ctype.h>

#include <fx_api.h>
#include <basework/log.h>
#include <subsys/fs/fs.h>
#include <drivers/blkdev.h>


#ifndef __ELASTERROR
#define __ELASTERROR (2000)
#endif

#define FX_PATH(_name) ((CHAR *)(_name) + fs->mountp_len)
#define FX_ERR(_err)   ((_err)? _FX_ERR(_err): 0)
#define _FX_ERR(_err) -(__ELASTERROR + (int)(_err))

struct dir_private {
    FX_LOCAL_PATH path;
    bool first;
};

struct filex_instance {
    FX_MEDIA media;
    char buffer[CONFIG_FS_FILEX_MEDIA_BUFFER_SIZE]  __rte_aligned(RTE_CACHE_LINE_SIZE);
};

extern UINT _fx_partition_offset_calculate(void  *partition_sector, UINT partition,
    ULONG *partition_start, ULONG *partition_size);

static FX_FILE filex_fds[CONFIG_FS_FILEX_NUM_FILES];
static struct object_pool filex_fds_pool;

static struct dir_private filex_dirs[CONFIG_FS_FILEX_NUM_DIRS];
static struct object_pool filex_dirs_pool;

static struct filex_instance filex_inst[CONFIG_FS_FILEX_NUM_INSTANCE];
static struct object_pool filex_inst_pool;

static int filex_media_write(FX_MEDIA *media_ptr, ULONG sector_start, ULONG sector_num) {
    struct device *dev = (struct device *)media_ptr->fx_media_driver_info;
    struct blkdev_req req;

    req.op     = BLKDEV_REQ_WRITE;
    req.blkno  = sector_start;
    req.blkcnt = sector_num;
    req.buffer = media_ptr->fx_media_driver_buffer;
    return blkdev_request(dev, &req);
}

static int filex_media_read(FX_MEDIA *media_ptr, ULONG sector_start, ULONG sector_num) {
    struct device *dev = (struct device *)media_ptr->fx_media_driver_info;
    struct blkdev_req req;

    req.op     = BLKDEV_REQ_READ;
    req.blkno  = sector_start;
    req.blkcnt = sector_num;
    req.buffer = media_ptr->fx_media_driver_buffer;
    return blkdev_request(dev, &req);
}

static void filex_fs_driver(FX_MEDIA *media_ptr) {
	switch (media_ptr->fx_media_driver_request) {
	case FX_DRIVER_READ: {
        int err = filex_media_read(media_ptr, 
            media_ptr->fx_media_driver_logical_sector + media_ptr->fx_media_hidden_sectors,
            media_ptr->fx_media_driver_sectors);
        media_ptr->fx_media_driver_status = err;
        break;
    }

	case FX_DRIVER_WRITE: {
        int err = filex_media_write(media_ptr, 
            media_ptr->fx_media_driver_logical_sector + media_ptr->fx_media_hidden_sectors,
            media_ptr->fx_media_driver_sectors);
        media_ptr->fx_media_driver_status = err;
        break;
    }

	case FX_DRIVER_FLUSH:
		/* Return driver success.  */
        device_control(media_ptr->fx_media_driver_info, BLKDEV_IOC_SYNC, NULL);
		media_ptr->fx_media_driver_status = FX_SUCCESS;
		break;

	case FX_DRIVER_ABORT:
		/* Return driver success.  */
		media_ptr->fx_media_driver_status = FX_SUCCESS;
		break;

	case FX_DRIVER_INIT:
		/* Successful driver request.  */
		media_ptr->fx_media_driver_status = FX_SUCCESS;
		break;

	case FX_DRIVER_UNINIT:
		/* Successful driver request.  */
		media_ptr->fx_media_driver_status = FX_SUCCESS;
		break;

	case FX_DRIVER_BOOT_READ: {
        ULONG partition_start, partition_size;

        int err = filex_media_read(media_ptr, 0,
            media_ptr->fx_media_driver_sectors);
		if (err == FX_SUCCESS) {
            err = _fx_partition_offset_calculate(media_ptr->fx_media_driver_buffer, 0,
                &partition_start, &partition_size);
            if (err) {
                pr_err("failed(%u) to read bootrec\n", err);
                media_ptr->fx_media_driver_status = FX_IO_ERROR;
                break;
            }
            if (partition_start) {
                /* Yes, now lets read the actual boot record.  */
                err = filex_media_read(media_ptr, partition_start,
                    media_ptr->fx_media_driver_sectors);
            }
        }
        media_ptr->fx_media_driver_status = err;
		break;
    }

	case FX_DRIVER_BOOT_WRITE: {
        int err = filex_media_write(media_ptr, 0,
            media_ptr->fx_media_driver_sectors);
        media_ptr->fx_media_driver_status = err;
        break;
    }
	default: 
		/* Invalid driver request.  */
		media_ptr->fx_media_driver_status = FX_IO_ERROR;
		break;
	}
}

static int filex_fs_open(struct fs_file *fp, const char *file_name, 
    fs_mode_t flags) {
    struct fs_class *fs = fp->vfs;
    bool created = false;
    UINT err;

    if (flags & FS_O_CREATE) {
        err = fx_file_create(fs->fs_data, FX_PATH(file_name));
        if (err == FX_SUCCESS)
            created = true;
        else if (err != FX_ALREADY_CREATED) {
            pr_err("%s: failed(%d) to create file(%s) \n", __func__, err, FX_PATH(file_name));
            return FX_ERR(err);
        }
    }

    int rw_flags = flags & (FS_O_WRITE | FS_O_READ | FS_O_TRUNC);
    if (!rw_flags && !created)
        return -EINVAL;

    FX_FILE *fxp = object_allocate(&filex_fds_pool);
    if (fxp) {
        UINT open_type;
#ifndef FX_DISABLE_FAST_OPEN
        open_type = (rw_flags == FS_O_READ)? FX_OPEN_FOR_READ_FAST: FX_OPEN_FOR_WRITE;
#else
        open_type = (rw_flags == FS_O_READ)? FX_OPEN_FOR_READ: FX_OPEN_FOR_WRITE;
#endif
        err = fx_file_open(fs->fs_data, fxp, FX_PATH(file_name), 
            open_type);
        if (err == FX_SUCCESS) {
            if ((rw_flags & FS_O_TRUNC) || ((rw_flags & FS_O_WRITE) && !created))
                fx_file_truncate(fxp, 0);

            fp->filep = fxp;
            return 0;
        }

        pr_dbg("%s: open file(%s) failed(%d)\n", __func__, FX_PATH(file_name), err);
        object_free(&filex_fds_pool, fxp);
        return _FX_ERR(err);
    }

    return -ENOMEM;
}

static int filex_fs_close(struct fs_file *fp) {
    FX_FILE *fxp = fp->filep;
    UINT err;

    err = fx_file_close(fxp);
    if (err == FX_SUCCESS) {
        object_free(&filex_fds_pool, fp->filep);
        return 0;
    }
    return _FX_ERR(err);
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
            err = fx_file_relative_seek(fxp, offset, FX_SEEK_BACK);
        break;
    case FS_SEEK_END:
        err = fx_file_relative_seek(fxp, offset, FX_SEEK_END);
        break;
    default:
        return -EINVAL;
    }

    return FX_ERR(err);
}

static off_t filex_fs_tell(struct fs_file *fp) {
    FX_FILE *fxp = fp->filep;
    return fxp->fx_file_current_file_size;
}

static int filex_fs_truncate(struct fs_file *fp, off_t length) {
    FX_FILE *fxp = fp->filep;
    UINT err;

    err = fx_file_truncate(fxp, length);
    return FX_ERR(err);
}

static int filex_fs_sync(struct fs_file *fp) {
    (void) fp;
    return -ENOSYS;
}

static int filex_fs_opendir(struct fs_dir *dp, const char *abs_path) {
    UINT err;
    
     struct dir_private *dir = object_allocate(&filex_dirs_pool);
    if (dir == NULL)
        return -ENOMEM;

    struct fs_class *fs = dp->vfs;
    err = fx_directory_local_path_set(fs->fs_data, &dir->path, 
        FX_PATH(abs_path));
    if (err == FX_SUCCESS) {
        dir->first = true;
        dp->dirp = dir;
        return 0;
    }

    object_free(&filex_dirs_pool, dir);
    return FX_ERR(err);
}

static int filex_fs_readdir(struct fs_dir *dp, struct fs_dirent *entry) {
    struct fs_class *fs = dp->vfs;
    struct dir_private *dir = dp->dirp;
    ULONG size;
    UINT attr;
    UINT err;

    if (!dir->first) {
        err = fx_directory_next_full_entry_find(fs->fs_data, (CHAR *)entry->name,
            &attr, &size, NULL, NULL, NULL, NULL, NULL, NULL);
    } else {
        dir->first = false;
        err = fx_directory_first_full_entry_find(fs->fs_data, (CHAR *)entry->name,
            &attr, &size, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    if (err == FX_SUCCESS) {
        if (!(attr & FX_DIRECTORY)) {
            entry->type = FS_DIR_ENTRY_FILE;
            entry->size = (size_t)size;
        } else {
            entry->type = FS_DIR_ENTRY_DIR;
            entry->size = 0;
        }
        return 0;    
    }

    return FX_ERR(err);
}

static int filex_fs_closedir(struct fs_dir *dp) {
    struct dir_private *dir = dp->dirp;
    if (dir) {
        struct fs_class *fs = dp->vfs;
        fx_directory_local_path_clear(fs->fs_data);
        object_free(&filex_dirs_pool, dir);
        dp->dirp = NULL;
    }

    return 0;
}

static int filex_fs_mount(struct fs_class *fs) {
    struct device *dev;
    UINT err;

    dev = device_find(fs->storage_dev);
    if (dev == NULL)
        return -ENODEV;

    UINT blksz = 0;
    device_control(dev, BLKDEV_IOC_GET_BLKSIZE, &blksz);
    if (blksz == 0 || blksz > 4096 || blksz > CONFIG_FS_FILEX_MEDIA_BUFFER_SIZE)
        return -EIO;

    struct filex_instance *fx = object_allocate(&filex_inst_pool);
    if (fx == NULL)
        return -ENOMEM;

    memset(&fx->media, 0, sizeof(fx->media));
    err = fx_media_open(&fx->media, (CHAR *)dev->name, filex_fs_driver, 
        dev, fx->buffer, sizeof(fx->buffer));
    if (err) {
        object_free(&filex_inst_pool, fx);
        return _FX_ERR(err);
    }

    fs->fs_data = &fx->media;

    return 0;
}

static int filex_fs_unmount(struct fs_class *fs) {
    UINT err;

    if (fs->fs_data == NULL)
        return -ENODATA;

    err = fx_media_close(fs->fs_data);
    if (err == FX_SUCCESS) {
        object_free(&filex_inst_pool, fs->fs_data);
        fs->fs_data = NULL;
    }
    return FX_ERR(err);
}

/* Filesystem operations */
static int filex_fs_mkdir(struct fs_class *fs, const char *abs_path) {
    UINT err;

    err = fx_directory_create(fs->fs_data, FX_PATH(abs_path));
    if (err == FX_ALREADY_CREATED)
        return 0;
    return FX_ERR(err);
}

static int filex_fs_unlink(struct fs_class *fs, const char *abs_path) {
    UINT attr, err;

    err = fx_file_attributes_read(fs->fs_data, FX_PATH(abs_path), &attr);
    if (err == FX_SUCCESS)
        err = fx_file_delete(fs->fs_data, FX_PATH(abs_path));
    else if (err == FX_NOT_A_FILE)
        err = fx_directory_delete(fs->fs_data, FX_PATH(abs_path));
    return FX_ERR(err);
}

static int filex_fs_rename(struct fs_class *fs, const char *from, const char *to) {
    UINT attr, err;

    err = fx_file_attributes_read(fs->fs_data, FX_PATH(from), &attr);
    if (err == FX_SUCCESS)
        err = fx_file_rename(fs->fs_data, FX_PATH(from), FX_PATH(to));
    else if (err == FX_NOT_A_FILE)
        err = fx_directory_rename(fs->fs_data, FX_PATH(from), FX_PATH(to));
    return FX_ERR(err);
}

static int filex_fs_stat(struct fs_class *fs, const char *abs_path, 
    struct fs_stat *stat) {
    ULONG size;
    UINT err;

    err = fx_directory_information_get(fs->fs_data, FX_PATH(abs_path), NULL, &size,
        NULL, NULL, NULL, NULL, NULL, NULL);
    
    if (err == FX_SUCCESS) {
        *stat = (struct fs_stat){0};
        stat->st_size = size;
        return 0;
    }

    return _FX_ERR(err);
}

static int filex_fs_statvfs(struct fs_class *fs, const char *abs_path, 
    struct fs_statvfs *stat) {
    return -ENOTSUP;
}

/*
 * cfg: vol=exfat fats=1 dirs=32 spc=32
 */
static bool parse_param(const char *cfg, const char *key, char *dst, 
    size_t maxsize, UINT *pval) {
    const char *src = strstr(cfg, key);
    
    if (src != NULL) {
        char *pdst = dst;

        src += strlen(key);
        while (*src && *src == ' ') src++;

        while (maxsize > 1 && *src && *src != ' ') {
            *pdst++ = *src++;
            maxsize--;
        }
        if (pdst - dst > 0) {
            *pdst = '\0';
            if (isdigit((int)(*dst)) && pval)
                *pval = (UINT)strtoul(dst, NULL, 10);
        }
        return true;
    }
    return false;
}

static int filex_fs_mkfs(const char *devname, void *cfg, int flags) {
    struct device *dev;
    UINT blkcnt = 0;
    UINT blksz = 0;
    UINT err;

    dev = device_find(devname);
    if (dev == NULL)
        return -ENODEV;

    device_control(dev, BLKDEV_IOC_GET_BLKCOUNT, &blkcnt);
    device_control(dev, BLKDEV_IOC_GET_BLKSIZE, &blksz);

    if (blkcnt == 0 || blksz == 0 || blksz > 4096)
        return -EINVAL;

    struct filex_instance *fx = object_allocate(&filex_inst_pool);
    if (fx == NULL)
        return -ENOMEM;

    /* 
     * Parse format options 
     */
    UINT directory_entries = 32;
    UINT sectors_per_cluster = 32; 
    UINT number_of_fats = 1;
    CHAR volume_name[64] = "exfat";
    if (cfg) {
        char numbuf[12];
        parse_param(cfg, "vol=", volume_name, sizeof(volume_name), NULL);
        parse_param(cfg, "fats=", numbuf, sizeof(numbuf), &number_of_fats);
        parse_param(cfg, "dirs=", numbuf, sizeof(numbuf), &directory_entries);
        parse_param(cfg, "spc=", numbuf, sizeof(numbuf), &sectors_per_cluster);
    }

    pr_info("format media(%s): volume_name(%s) number_of_fats(%u) directory_entries(%u)"
        "sectors_per_cluster(%u)\n", devname,
        volume_name, number_of_fats, directory_entries, sectors_per_cluster);

    memset(&fx->media, 0, sizeof(fx->media));
#ifdef FX_ENABLE_EXFAT
    err = fx_media_exFAT_format(&fx->media,
                          filex_fs_driver,         // Driver entry
                          dev,        // RAM disk memory pointer
                          (UCHAR *)fx->buffer,           // Media buffer pointer
                          sizeof(fx->buffer),   // Media buffer size
                          volume_name,          // Volume Name
                          1,                      // Number of FATs
                          0,                      // Hidden sectors
                          blkcnt,                    // Total sectors
                          blksz,                    // Sector size
                          sectors_per_cluster,                      // exFAT Sectors per cluster
                          12345,                  // Volume ID
                          1);                     // Boundary unit

#else /* !FX_ENABLE_EXFAT */
    err = fx_media_format(&fx->media,
                    filex_fs_driver,               // Driver entry
                    dev, // RAM disk memory pointer
                    (UCHAR *)fx->buffer,     // Media buffer pointer
                    sizeof(fx->buffer),   // Media buffer size
                    volume_name,                // Volume Name
                    number_of_fats,                   // Number of FATs
                    directory_entries,               // Directory Entries
                    0,                   // Hidden sectors
                    blkcnt,               // Total sectors
                    blksz,             // Sector size
                    sectors_per_cluster,              // Sectors per cluster
                    1,                            // Heads
                    1);               // Sectors per track
#endif /* FX_ENABLE_EXFAT */
    object_free(&filex_inst_pool, fx);

    return FX_ERR(err);
}

static int filex_flush(struct fs_class *fs) {
    UINT err = fx_media_flush(fs->fs_data);
    return FX_ERR(err);
}

static const struct fs_operations fs_ops = {
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
    .mkfs     = filex_fs_mkfs,
    .flush    = filex_flush
};

static int fs_filex_init(void) {
    fx_system_initialize();

    object_pool_initialize(&filex_inst_pool, filex_inst, 
        sizeof(filex_inst), sizeof(filex_inst[0]));

    object_pool_initialize(&filex_fds_pool, filex_fds, 
        sizeof(filex_fds), sizeof(filex_fds[0]));

    object_pool_initialize(&filex_dirs_pool, filex_dirs, 
        sizeof(filex_dirs), sizeof(filex_dirs[0]));

    return fs_register(FS_EXFATFS, &fs_ops);
}

SYSINIT(fs_filex_init, SI_FILESYSTEM_LEVEL, 00);
