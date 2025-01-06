/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 *
 * Virtual Filesystem (borrowed from zephyr)
 */

#define pr_fmt(fmt) "[fs]: "fmt
#include <errno.h>
#include <string.h>

#include "tx_api.h"
#include "subsys/fs/fs.h"
#include "basework/container/list.h"
#include "basework/log.h"


struct fs_manager {
    struct rte_list list;
    struct rte_list mnt_list;
    TX_MUTEX mtx;
};

struct fs_manager fs_manager;

static struct fs_class *fs_type_registered_get(int type) {
    struct fs_class *iter;
    rte_list_foreach_entry(iter, &fs_manager.list, node) {
        if (iter->type == type)
            return iter;
    }
    return NULL;
}

static struct fs_class *fs_mntp_mounted_get(const char *mnt) {
    struct fs_class *iter;
    rte_list_foreach_entry(iter, &fs_manager.mnt_list, node) {
        if (!strcmp(mnt, iter->mnt_point))
            return iter;
    }
    return NULL;
}

static int fs_get_mnt_point(struct fs_class **mnt_pntp, const char *name,
	size_t *match_len) {
	struct fs_class *mnt_p = NULL, *itr;
	size_t longest_match = 0;
	size_t len, name_len = strlen(name);
	struct rte_list *node;

	tx_mutex_get(&fs_manager.mtx, TX_WAIT_FOREVER);
	rte_list_foreach(node, &fs_manager.mnt_list) {
		itr = rte_container_of(node, struct fs_class, node);
		len = itr->mountp_len;

		/*
		 * Move to next node if mount point length is
		 * shorter than longest_match match or if path
		 * name is shorter than the mount point name.
		 */
		if ((len < longest_match) || (len > name_len))
			continue;

		/*
		 * Move to next node if name does not have a directory
		 * separator where mount point name ends.
		 */
		if ((len > 1) && (name[len] != '/') && (name[len] != '\0'))
			continue;

		/* Check for mount point match */
		if (strncmp(name, itr->mnt_point, len) == 0) {
			mnt_p = itr;
			longest_match = len;
		}
	}
	tx_mutex_put(&fs_manager.mtx);

	if (mnt_p == NULL) {
		return -ENOENT;
	}

	*mnt_pntp = mnt_p;
	if (match_len) {
		*match_len = mnt_p->mountp_len;
	}

	return 0;
}

/* File operations */
int fs_open(struct fs_file *fp, const char *file_name, fs_mode_t flags) {
	struct fs_class *fs;
	int rc = -EINVAL;
	bool truncate_file = false;

	if ((file_name == NULL) || (strlen(file_name) <= 1) || (file_name[0] != '/')) {
		pr_err("invalid file name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&fs, file_name, NULL);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	if (((fs->flags & FS_MOUNT_FLAG_READ_ONLY) != 0) &&
		(flags & FS_O_CREATE || flags & FS_O_WRITE)) {
		return -EROFS;
	}

	if (fs->fs_ops.open == NULL)
		return -ENOTSUP;

	if ((flags & FS_O_TRUNC) != 0) {
		if ((flags & FS_O_WRITE) == 0) {
			/** Truncate not allowed when file is not opened for write */
			pr_err("file should be opened for write to truncate!!");
			return -EACCES;
		}

		if (fs->fs_ops.truncate == NULL) {
			pr_err("file truncation not supported!!");
			return -ENOTSUP;
		}

		truncate_file = true;
	}

	fp->vfs = fs;
	rc = fs->fs_ops.open(fp, file_name, flags);
	if (rc < 0) {
		pr_err("file open error (%d)", rc);
		fp->vfs = NULL;
		return rc;
	}

	/* Copy flags to fp for use with other fs_ API calls */
	fp->flags = flags;

	if (truncate_file) {
		/* Truncate the opened file to 0 length */
		rc = fs->fs_ops.truncate(fp, 0);
		if (rc < 0) {
			pr_err("file truncation failed (%d)", rc);
			fp->vfs = NULL;
			return rc;
		}
	}

	return rc;
}

int fs_close(struct fs_file *fp) {
	if (rte_unlikely(fp->vfs == NULL))
		return 0;

	if (fp->vfs->fs_ops.close == NULL) {
		return -ENOTSUP;
	}

	int rc = fp->vfs->fs_ops.close(fp);
	if (rc < 0) {
		pr_err("file close error (%d)", rc);
		return rc;
	}

	fp->vfs = NULL;

	return rc;
}

ssize_t fs_read(struct fs_file *fp, void *ptr, size_t size) {
	if (rte_unlikely(fp->vfs == NULL))
		return -EBADF;

	int rc = fp->vfs->fs_ops.read(fp, ptr, size);
	if (rc < 0) {
		pr_err("file read error (%d)", rc);
	}

	return rc;
}

ssize_t fs_write(struct fs_file *fp, const void *ptr, size_t size) {
	if (rte_unlikely(fp->vfs == NULL))
		return -EBADF;

	int rc = fp->vfs->fs_ops.write(fp, ptr, size);
	if (rc < 0) {
		pr_err("file write error (%d)", rc);
	}

	return rc;
}

int fs_seek(struct fs_file *fp, off_t offset, int whence) {
	if (rte_unlikely(fp->vfs == NULL))
		return -EBADF;

	if (rte_unlikely(fp->vfs->fs_ops.lseek == NULL))
		return -ENOTSUP;

	int rc = fp->vfs->fs_ops.lseek(fp, offset, whence);
	if (rc < 0) {
		pr_err("file seek error (%d)", rc);
	}

	return rc;
}

off_t fs_tell(struct fs_file *fp) {
	if (rte_unlikely(fp->vfs == NULL))
		return -EBADF;

	if (fp->vfs->fs_ops.tell == NULL)
		return -ENOTSUP;

	int rc = fp->vfs->fs_ops.tell(fp);
	if (rc < 0) {
		pr_err("file tell error (%d)", rc);
	}

	return rc;
}

int fs_truncate(struct fs_file *fp, off_t length) {
	if (rte_unlikely(fp->vfs == NULL))
		return -EBADF;

	if (fp->vfs->fs_ops.truncate == NULL)
		return -ENOTSUP;

	int rc = fp->vfs->fs_ops.truncate(fp, length);
	if (rc < 0) {
		pr_err("file truncate error (%d)", rc);
	}

	return rc;
}

int fs_sync(struct fs_file *fp) {
	if (rte_unlikely(fp->vfs == NULL))
		return -EBADF;

	if (fp->vfs->fs_ops.sync == NULL)
		return -ENOTSUP;

	int rc = fp->vfs->fs_ops.sync(fp);
	if (rc < 0) {
		pr_err("file sync error (%d)", rc);
	}

	return rc;
}

/* Directory operations */
int fs_opendir(struct fs_dir *dp, const char *abs_path) {
	struct fs_class *mp;
	int rc = -EINVAL;

	if ((abs_path == NULL) || (strlen(abs_path) < 1) || (abs_path[0] != '/')) {
		pr_err("invalid directory name!!");
		return -EINVAL;
	}

	if (dp->vfs != NULL || dp->dirp != NULL)
		return -EBUSY;

	if (strcmp(abs_path, "/") == 0) {
		/* Open VFS root dir, marked by dp->vfs == NULL */
		tx_mutex_get(&fs_manager.mtx, TX_WAIT_FOREVER);

		dp->vfs = NULL;
        if (rte_list_empty(&fs_manager.mnt_list))
            dp->dirp = NULL;
        else
            dp->dirp = fs_manager.mnt_list.next;

		tx_mutex_put(&fs_manager.mtx);

		return 0;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	if (mp->fs_ops.opendir == NULL)
		return -ENOTSUP;

	dp->vfs = mp;
	rc = dp->vfs->fs_ops.opendir(dp, abs_path);
	if (rc < 0) {
		dp->vfs = NULL;
		dp->dirp = NULL;
		pr_err("directory open error (%d)", rc);
	}

	return rc;
}

int fs_readdir(struct fs_dir *dp, struct fs_dirent *entry) {
	if (dp->vfs) {
		/* Delegate to mounted filesystem */
		int rc = -EINVAL;

		if (dp->vfs->fs_ops.readdir == NULL)
			return -ENOTSUP;

		/* Loop until error or not special directory */
		while (true) {
			rc = dp->vfs->fs_ops.readdir(dp, entry);
			if (rc < 0) 
				break;
			
			if (entry->name[0] == 0) 
				break;
			
			if (entry->type != FS_DIR_ENTRY_DIR) 
				break;
			
			if ((strcmp(entry->name, ".") != 0) && (strcmp(entry->name, "..") != 0)) 
				break;
		}
		if (rc < 0) {
			pr_err("directory read error (%d)", rc);
		}

		return rc;
	}

	/* VFS root dir */
	if (dp->dirp == NULL) {
		/* No more entries */
		entry->name[0] = 0;
		return 0;
	}

	/* Find the current and next entries in the mount point dlist */
	struct rte_list *node, *next = NULL;
	bool found = false;

	tx_mutex_get(&fs_manager.mtx, TX_WAIT_FOREVER);

	rte_list_foreach(node, &fs_manager.mnt_list) {
		if (node == dp->dirp) {
			found = true;

			/* Pull info from current entry */
			struct fs_class *mnt;

			mnt = rte_container_of(node, struct fs_class, node);

			entry->type = FS_DIR_ENTRY_DIR;
			strncpy(entry->name, mnt->mnt_point + 1, sizeof(entry->name) - 1);
			entry->name[sizeof(entry->name) - 1] = 0;
			entry->size = 0;

			/* Save pointer to the next one, for later */
            next = node->next;
            if (next == &fs_manager.mnt_list)
			    next = NULL;
			break;
		}
	}

	tx_mutex_put(&fs_manager.mtx);

	if (!found) {
		/* Current entry must have been removed before this
		 * call to readdir -- return an error
		 */
		return -ENOENT;
	}

	dp->dirp = next;
	return 0;
}

int fs_closedir(struct fs_dir *dp) {
	int rc = -EINVAL;

	if (dp->vfs == NULL) {
		/* VFS root dir */
		dp->dirp = NULL;
		return 0;
	}

	if (dp->vfs->fs_ops.closedir == NULL)
		return -ENOTSUP;

	rc = dp->vfs->fs_ops.closedir(dp);
	if (rc < 0) {
		pr_err("directory close error (%d)", rc);
		return rc;
	}

	dp->vfs = NULL;
	dp->dirp = NULL;
	return rc;
}

/* Filesystem operations */
int fs_mkdir(const char *abs_path) {
	struct fs_class *mp;
	int rc = -EINVAL;

	if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		pr_err("invalid directory name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	if (mp->flags & FS_MOUNT_FLAG_READ_ONLY)
		return -EROFS;

	if (mp->fs_ops.mkdir == NULL)
		return -ENOTSUP;

	rc = mp->fs_ops.mkdir(mp, abs_path);
	if (rc < 0) {
		pr_err("failed to create directory (%d)", rc);
	}

	return rc;
}

int fs_unlink(const char *abs_path) {
	struct fs_class *mp;
	int rc = -EINVAL;

	if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		pr_err("invalid file name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	if (mp->flags & FS_MOUNT_FLAG_READ_ONLY)
		return -EROFS;

	if (mp->fs_ops.unlink == NULL)
		return -ENOTSUP;

	rc = mp->fs_ops.unlink(mp, abs_path);
	if (rc < 0) {
		pr_err("failed to unlink path (%d)", rc);
	}

	return rc;
}

int fs_rename(const char *from, const char *to) {
	struct fs_class *mp;
	size_t match_len;
	int rc = -EINVAL;

	if ((from == NULL) || (strlen(from) <= 1) || (from[0] != '/') || (to == NULL) ||
		(strlen(to) <= 1) || (to[0] != '/')) {
		pr_err("invalid file name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, from, &match_len);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	if (mp->flags & FS_MOUNT_FLAG_READ_ONLY)
		return -EROFS;

	/* Make sure both files are mounted on the same path */
	if (strncmp(from, to, match_len) != 0) {
		pr_err("mount point not same!!");
		return -EINVAL;
	}

	if (mp->fs_ops.rename == NULL)
		return -ENOTSUP;

	rc = mp->fs_ops.rename(mp, from, to);
	if (rc < 0) {
		pr_err("failed to rename file or dir (%d)", rc);
	}

	return rc;
}

int fs_stat(const char *abs_path, struct fs_stat *stat) {
	struct fs_class *mp;
	int rc = -EINVAL;

	if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		pr_err("invalid file or dir name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	if (mp->fs_ops.stat == NULL)
		return -ENOTSUP;

	rc = mp->fs_ops.stat(mp, abs_path, stat);
	if (rc == -ENOENT) {
		/* File doesn't exist, which is a valid stat response */
	} else if (rc < 0) {
		pr_err("failed get file or dir stat (%d)", rc);
	}
	return rc;
}

int fs_statvfs(const char *abs_path, struct fs_statvfs *stat) {
	struct fs_class *mp;
	int rc;

	if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		pr_err("invalid file or dir name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&mp, abs_path, NULL);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	if (mp->fs_ops.statvfs == NULL)
		return -ENOTSUP;

	rc = mp->fs_ops.statvfs(mp, abs_path, stat);
	if (rc < 0) {
		pr_err("failed get file or dir stat (%d)", rc);
	}

	return rc;
}

int fs_mount(const char *mnt, void *storage_dev, int type, 
    unsigned int options, void *fs_data) {
	struct fs_class *itr, *fs;
	struct rte_list *node;
	int rc = -EINVAL;
	size_t len = 0;

	/* Do all the mp checks prior to locking the mutex on the file
	 * subsystem.
	 */
	if ((mnt == NULL) || (storage_dev == NULL)) {
		pr_err("mount point not initialized!!");
		return -EINVAL;
	}

	len = strlen(mnt);
	if ((len <= 1) || (mnt[0] != '/')) {
		pr_err("invalid mount point!!");
		return -EINVAL;
	}

	tx_mutex_get(&fs_manager.mtx, TX_WAIT_FOREVER);

	/* Get file system information */
	fs = fs_type_registered_get(type);
	if (fs == NULL) {
		pr_err("requested file system type not registered!!");
		rc = -ENOENT;
		goto mount_err;
	}

    if (fs->mnt_point != NULL) {
		pr_err("file system already mounted!!");
		rc = -EBUSY;
        goto mount_err;
    }

	/* Check if mount point already exists */
	rte_list_foreach(node, &fs_manager.mnt_list) {
		itr = rte_container_of(node, struct fs_class, node);
		/* continue if length does not match */
		if (len != itr->mountp_len)
			continue;

		if (fs_data == itr->fs_data) {
			pr_err("file system already mounted!!");
			rc = -EBUSY;
			goto mount_err;
		}

		if (strncmp(mnt, itr->mnt_point, len) == 0) {
			pr_err("mount point already exists!!");
			rc = -EBUSY;
			goto mount_err;
		}
	}

	if (fs->fs_ops.mount == NULL) {
		pr_err("fs type %d does not support mounting", type);
		rc = -ENOTSUP;
		goto mount_err;
	}

	if (fs->fs_ops.unmount == NULL) {
		pr_warn("mount path %s is not unmountable", mnt);
	}

    fs->mnt_point = mnt;
    fs->storage_dev = storage_dev;
    fs->flags = (uint8_t)options;
    fs->fs_data = fs_data;
	rc = fs->fs_ops.mount(fs);
	if (rc < 0) {
        fs->mnt_point = NULL;
        fs->storage_dev = NULL;
        fs->flags = 0;
        fs->fs_data = NULL;
		pr_err("fs mount error (%d)", rc);
		goto mount_err;
	}

	/* Update mount point data and append it to the list */
	fs->mountp_len = len;

    rte_list_del(&fs->node);
    rte_list_add_tail(&fs->node, &fs_manager.mnt_list);
	pr_dbg("fs mounted at %s", fs->mnt_point);

mount_err:
	tx_mutex_put(&fs_manager.mtx);
	return rc;
}

int fs_mkfs(int fs_type, const char *dev, void *cfg, int flags) {
	int rc = -EINVAL;
	struct fs_class *fs;

	tx_mutex_get(&fs_manager.mtx, TX_WAIT_FOREVER);

	/* Get file system information */
	fs = fs_type_registered_get(fs_type);
	if (fs == NULL) {
		pr_err("fs type %d not registered!!", fs_type);
		rc = -ENOENT;
		goto mount_err;
	}

	if (fs->fs_ops.mkfs == NULL) {
		pr_err("fs type %d does not support mkfs", fs_type);
		rc = -ENOTSUP;
		goto mount_err;
	}

	rc = fs->fs_ops.mkfs(fs, dev, cfg, flags);
	if (rc < 0) {
		pr_err("mkfs error (%d)", rc);
		goto mount_err;
	}

mount_err:
	tx_mutex_put(&fs_manager.mtx);
	return rc;
}

int fs_unmount(const char *mnt_point) {
	int rc = -EINVAL;

	if (mnt_point == NULL)
		return rc;

	tx_mutex_get(&fs_manager.mtx, TX_WAIT_FOREVER);

    struct fs_class *fs = fs_mntp_mounted_get(mnt_point);
	if (fs == NULL) {
		pr_err("fs not mounted (mp == %p)", fs);
		goto unmount_err;
	}

	if (fs->fs_ops.unmount == NULL) {
		pr_err("fs unmount not supported!!");
		rc = -ENOTSUP;
		goto unmount_err;
	}

	rc = fs->fs_ops.unmount(fs);
	if (rc < 0) {
		pr_err("fs unmount error (%d)", rc);
		goto unmount_err;
	}

	/* clear file system interface */

	/* remove mount node from the list */
	rte_list_del(&fs->node);
	pr_dbg("fs unmounted from %s", fs->mnt_point);

unmount_err:
	tx_mutex_put(&fs_manager.mtx);
	return rc;
}

/* Register File system */
int fs_register(int type, struct fs_class *fs) {
    struct fs_class *iter;
	int rc = 0;

    if (fs == NULL)
        return -EINVAL;

    /* The filesystem must be support read operation */
    if (fs->fs_ops.read == NULL)
        return -EINVAL;

    tx_mutex_get(&fs_manager.mtx, TX_WAIT_FOREVER);

    rte_list_foreach_entry(iter, &fs_manager.list, node) {
        if (iter->type == type) {
            rc = -EALREADY;
            goto _out;
        }
    }
    rte_list_foreach_entry(iter, &fs_manager.mnt_list, node) {
        if (iter->type == type) {
            rc = -EALREADY;
            goto _out;
        }
    }

    fs->type = type;
    rte_list_add_tail(&fs->node, &fs_manager.list);
	pr_dbg("fs register %d: %d", type, rc);

_out:
    tx_mutex_put(&fs_manager.mtx);
	return rc;
}

/* Unregister File system */
int fs_unregister(int type) {
    struct fs_class *iter;
	int rc = 0;

	tx_mutex_get(&fs_manager.mtx, TX_WAIT_FOREVER);

    rte_list_foreach_entry(iter, &fs_manager.list, node) {
        if (iter->type == type) {
            rc = fs_unmount(iter->mnt_point);
            if (!rc)
                rte_list_del(&iter->node);
            break;
        }
    }

	tx_mutex_put(&fs_manager.mtx);

	pr_dbg("fs unregister %d: %d", type, rc);
	return rc;
}

static int fs_init(void) {
    RTE_INIT_LIST(&fs_manager.list);
    RTE_INIT_LIST(&fs_manager.mnt_list);
    tx_mutex_create(&fs_manager.mtx, "fs", TX_INHERIT);
    return 0;
}

SYSINIT(fs_init, SI_PREDRIVER_LEVEL, 10);
