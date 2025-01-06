/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 *
 */

#include <errno.h>
#include "tx_api.h"
#include "fx_api.h"
#include "drivers/blkdev.h"

#define FS_PRIVATE_EXTENSION \
    FX_MEDIA media; \
    struct object_pool pool;
    
#include "subsys/fs/fs.h"


struct dir_private {
    FX_LOCAL_PATH path;
    bool first;
};

#ifndef __ELASTERROR
#define __ELASTERROR (2000)
#endif

#define FX_ERR(_err)  ((_err)? _FX_ERR(_err): 0)
#define _FX_ERR(_err) -(__ELASTERROR + (int)(_err))

#ifndef CONFIG_FILEX_MEDIA_BUFFER_SIZE
#define CONFIG_FILEX_MEDIA_BUFFER_SIZE 4096
#endif
#ifndef CONFIG_FILEX_MAX_FILES
#define CONFIG_FILEX_MAX_FILES 1
#endif

static char media_buffer[CONFIG_FILEX_MEDIA_BUFFER_SIZE] __rte_aligned(RTE_CACHE_LINE_SIZE);
static FX_FILE filex_fds[CONFIG_FILEX_MAX_FILES];

static void filex_fs_driver(FX_MEDIA *media_ptr) {
	UCHAR *buffer;
	UINT bytes_per_sector;

	/* Process the driver request specified in the media control block.  */
	switch (media_ptr->fx_media_driver_request) {
	case FX_DRIVER_READ: {
        struct blkdev_req req;
        int err;

        req.op = BLKDEV_REQ_READ;
        req.blkno = media_ptr->fx_media_driver_logical_sector + media_ptr->fx_media_hidden_sectors;
        req.blkcnt = media_ptr->fx_media_driver_sectors;
        req.buffer = media_ptr->fx_media_driver_buffer;
        err = blkdev_request(media_ptr->fx_media_driver_info, &req);
        media_ptr->fx_media_driver_status = err;
        break;
    }

	case FX_DRIVER_WRITE: {
        struct blkdev_req req;
        int err;

        req.op = BLKDEV_REQ_WRITE;
        req.blkno = media_ptr->fx_media_driver_logical_sector + media_ptr->fx_media_hidden_sectors;
        req.blkcnt = media_ptr->fx_media_driver_sectors;
        req.buffer = media_ptr->fx_media_driver_buffer;
        err = blkdev_request(media_ptr->fx_media_driver_info, &req);
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
		/* FLASH drivers are responsible for setting several fields in the
		   media structure, as follows:

				media_ptr -> fx_media_driver_free_sector_update
				media_ptr -> fx_media_driver_write_protect

		   The fx_media_driver_free_sector_update flag is used to instruct
		   FileX to inform the driver whenever sectors are not being used.
		   This is especially useful for FLASH managers so they don't have
		   maintain mapping for sectors no longer in use.

		   The fx_media_driver_write_protect flag can be set anytime by the
		   driver to indicate the media is not writable.  Write attempts made
		   when this flag is set are returned as errors.  */

		/* Perform basic initialization here... since the boot record is going
		   to be read subsequently and again for volume name requests.  */

		/* Successful driver request.  */
		media_ptr->fx_media_driver_status = FX_SUCCESS;
		break;

	case FX_DRIVER_UNINIT:
		/* Successful driver request.  */
		media_ptr->fx_media_driver_status = FX_SUCCESS;
		break;

	case FX_DRIVER_BOOT_READ: {
        struct blkdev_req req;
        int err;

        req.op = BLKDEV_REQ_READ;
        req.blkno = 0;
        req.blkcnt = 1;
        req.buffer = media_ptr->fx_media_driver_buffer;
        err = blkdev_request(media_ptr->fx_media_driver_info, &req);
		if (err == FX_SUCCESS) {
			/* Calculate the RAM disk boot sector offset, which is at the very beginning of
			the RAM disk. Note the RAM disk memory is pointed to by the
			fx_media_driver_info pointer, which is supplied by the application in the
			call to fx_media_open.  */
			buffer = (UCHAR *)media_ptr->fx_media_driver_buffer;

			/* For RAM driver, determine if the boot record is valid.  */
			if ((buffer[0] != (UCHAR)0xEB) ||
				((buffer[1] != (UCHAR)0x34) && (buffer[1] != (UCHAR)0x76)) ||
				(buffer[2] != (UCHAR)0x90)) {
				/* Invalid boot record, return an error!  */
				media_ptr->fx_media_driver_status = FX_MEDIA_INVALID;
				return;
			}

			/* For RAM disk only, pickup the bytes per sector.  */
			bytes_per_sector = _fx_utility_16_unsigned_read(&buffer[FX_BYTES_SECTOR]);

			/* Ensure this is less than the media memory size.  */
			if (bytes_per_sector > media_ptr->fx_media_memory_size) {
				media_ptr->fx_media_driver_status = FX_BUFFER_ERROR;
				break;
			}
		}
        media_ptr->fx_media_driver_status = err;
		break;
    }

	case FX_DRIVER_BOOT_WRITE: {
        struct blkdev_req req;
        int err;

        req.op = BLKDEV_REQ_WRITE;
        req.blkno = 0;
        req.blkcnt = 1;
        req.buffer = media_ptr->fx_media_driver_buffer;
        err = blkdev_request(media_ptr->fx_media_driver_info, &req);
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
        err = fx_file_create(&fs->media, (CHAR *)file_name);
        if (err != FX_SUCCESS)
            return FX_ERR(err);
        created = true;
    }

    int rw_flags = flags & (FS_O_WRITE|FS_O_READ);
    if (!rw_flags && !created)
        return -EINVAL;

    FX_FILE *fxp = object_allocate(&fs->pool);
    if (fxp) {
        err = fx_file_open(&fs->media, fxp, (CHAR *)file_name, 
            rw_flags == FS_O_READ? FX_OPEN_FOR_READ: FX_OPEN_FOR_WRITE);
        if (err == FX_SUCCESS) {
            fp->filep = fxp;
            return 0;
        }

        object_free(&fs->pool, fxp);
        return _FX_ERR(err);
    }

    return -ENOMEM;
}

static int filex_fs_close(struct fs_file *fp) {
    FX_FILE *fxp = fp->filep;
    UINT err;

    err = fx_file_close(fxp);
    if (err == FX_SUCCESS) {
        object_free(&fp->vfs->pool, fp->filep);
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
            err = fx_file_relative_seek(fxp, -offset, FX_SEEK_BACK);
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
    return -ENOTSUP;
}

static int filex_fs_opendir(struct fs_dir *dp, const char *abs_path) {
    struct fs_class *fs = dp->vfs;
    struct dir_private *dir;
    int err;
    
    dir = kmalloc(sizeof(*dir), 0);
    if (dir == NULL)
        return -ENOMEM;

    err = fx_directory_local_path_set(&fs->media, &dir->path, (CHAR *)abs_path);
    if (err == FX_SUCCESS) {
        dir->first = true;
        dp->dirp = dir;
        return 0;
    }

    kfree(dir);
    return FX_ERR(err);
}

static int filex_fs_readdir(struct fs_dir *dp, struct fs_dirent *entry) {
    struct fs_class *fs = dp->vfs;
    struct dir_private *dir = dp->dirp;
    ULONG size;
    UINT attr;
    UINT err;

    if (!dir->first) {
        err = fx_directory_next_full_entry_find(&fs->media, (CHAR *)entry->name,
            &attr, &size, NULL, NULL, NULL, NULL, NULL, NULL);
    } else {
        dir->first = false;
        err = fx_directory_first_full_entry_find(&fs->media, (CHAR *)entry->name,
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
        fx_directory_local_path_clear(&fs->media);
        kfree(dir);
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

    err = fx_media_open(&fs->media, (CHAR *)dev->name, filex_fs_driver, 
        dev, media_buffer, sizeof(media_buffer));

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

static int filex_fs_stat(struct fs_class *fs, const char *abs_path, 
    struct fs_stat *stat) {
    ULONG size;
    UINT err;

    err = fx_directory_information_get(&fs->media, (CHAR *)abs_path, NULL, &size,
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

static int filex_fs_mkfs(struct fs_class *fs, const char *devname, 
    void *cfg, int flags) {
    struct device *dev;
    UINT blkcnt = 0;
    UINT blksz = 0;
    UINT err;

    dev = device_find(devname);
    if (dev == NULL)
        return -ENODEV;

    device_control(dev, BLKDEV_IOC_GET_BLKCOUNT, &blkcnt);
    device_control(dev, BLKDEV_IOC_GET_BLKSIZE, &blksz);

    if (blkcnt == 0 || blksz == 0)
        return -EINVAL;

    err = fx_media_format(&fs->media,
                    filex_fs_driver,               // Driver entry
                    dev_get_private(dev), // RAM disk memory pointer
                    (UCHAR *)media_buffer,     // Media buffer pointer
                    sizeof(media_buffer),   // Media buffer size
                    "exfat",                // Volume Name
                    1,                   // Number of FATs
                    32,               // Directory Entries
                    0,                   // Hidden sectors
                    blkcnt,               // Total sectors
                    blksz,             // Sector size
                    16,              // Sectors per cluster
                    1,                            // Heads
                    1);               // Sectors per track

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

    object_pool_initialize(&filex_fs.pool, filex_fds, 
        sizeof(filex_fds), sizeof(filex_fds[0]));
    return fs_register(FS_EXFATFS, &filex_fs);
}

SYSINIT(fs_filex_init, SI_FILESYSTEM_LEVEL, 00);
