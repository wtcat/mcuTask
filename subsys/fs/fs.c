/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 *
 * Virtual Filesystem (borrowed from zephyr)
 */

#include <stdlib.h>
#define pr_fmt(fmt) "[fs]: " fmt"\n"
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

struct registry_entry {
	int type;
	const struct fs_operations *fs_ops;
};

static struct registry_entry registry[FS_MAX];
static struct fs_manager fs_manager;


static void fs_operations_copy(struct fs_operations *to, 
	const struct fs_operations *from) {
	size_t n = sizeof(struct fs_operations) / sizeof(void *);
	void **pdst = (void **)to;
	void *const *psrc = (void **)from;
	
	*to = _fs_default_operation;

	for (size_t i = 0; i < n; i++) {
		if (psrc[i] != NULL)
			pdst[i] = psrc[i];
	}
}

static int registry_add(int type, const struct fs_operations *fs_ops) {
	for (size_t i = 0; i < rte_array_size(registry); ++i) {
		struct registry_entry *ep = &registry[i];

		if (ep->fs_ops == NULL) {
			ep->type   = type;
			ep->fs_ops = fs_ops;
			return 0;
		}
	}
	return -ENOSPC;
}

static struct registry_entry *registry_find(int type) {
	for (size_t i = 0; i < rte_array_size(registry); ++i) {
		struct registry_entry *ep = &registry[i];
		if (ep->fs_ops != NULL && ep->type == type)
			return ep;
	}
	return NULL;
}

static const struct fs_operations *fs_type_get(int type) {
	struct registry_entry *ep = registry_find(type);

	return (ep != NULL)? ep->fs_ops: NULL;
}

static struct fs_class *fs_mounted_get(const char *mnt) {
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
	if (rc < 0)
		pr_err("file read error (%d)", rc);

	return rc;
}

ssize_t fs_write(struct fs_file *fp, const void *ptr, size_t size) {
	if (rte_unlikely(fp->vfs == NULL))
		return -EBADF;

	int rc = fp->vfs->fs_ops.write(fp, ptr, size);
	if (rc < 0)
		pr_err("file write error (%d)", rc);

	return rc;
}

int fs_seek(struct fs_file *fp, off_t offset, int whence) {
	if (rte_unlikely(fp->vfs == NULL))
		return -EBADF;

	int rc = fp->vfs->fs_ops.lseek(fp, offset, whence);
	if (rc < 0)
		pr_err("file seek error (%d)", rc);

	return rc;
}

off_t fs_tell(struct fs_file *fp) {
	if (rte_unlikely(fp->vfs == NULL))
		return -EBADF;

	int rc = fp->vfs->fs_ops.tell(fp);
	if (rc < 0)
		pr_err("file tell error (%d)", rc);

	return rc;
}

int fs_truncate(struct fs_file *fp, off_t length) {
	if (rte_unlikely(fp->vfs == NULL))
		return -EBADF;

	int rc = fp->vfs->fs_ops.truncate(fp, length);
	if (rc < 0)
		pr_err("file truncate error (%d)", rc);

	return rc;
}

int fs_sync(struct fs_file *fp) {
	if (rte_unlikely(fp->vfs == NULL))
		return -EBADF;

	int rc = fp->vfs->fs_ops.sync(fp);
	if (rc < 0)
		pr_err("file sync error (%d)", rc);

	return rc;
}

/* Directory operations */
int fs_opendir(struct fs_dir *dp, const char *abs_path) {
	struct fs_class *fs;
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

	rc = fs_get_mnt_point(&fs, abs_path, NULL);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	dp->vfs = fs;
	rc = dp->vfs->fs_ops.opendir(dp, abs_path);
	if (rc < 0) {
		dp->vfs = NULL;
		dp->dirp = NULL;
		pr_err("directory(%s) open error (%d)", abs_path, rc);
	}

	return rc;
}

int fs_readdir(struct fs_dir *dp, struct fs_dirent *entry) {
	if (dp->vfs) {
		/* Delegate to mounted filesystem */
		int rc = -EINVAL;

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
		if (rc < 0)
			pr_err("directory read error (%d)\n", rc);

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
	struct fs_class *fs;
	int rc = -EINVAL;

	if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		pr_err("invalid directory name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&fs, abs_path, NULL);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	if (fs->flags & FS_MOUNT_FLAG_READ_ONLY)
		return -EROFS;

	rc = fs->fs_ops.mkdir(fs, abs_path);
	if (rc < 0)
		pr_err("failed to create directory (%d)", rc);

	return rc;
}

int fs_unlink(const char *abs_path) {
	struct fs_class *fs;
	int rc = -EINVAL;

	if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		pr_err("invalid file name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&fs, abs_path, NULL);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	if (fs->flags & FS_MOUNT_FLAG_READ_ONLY)
		return -EROFS;

	rc = fs->fs_ops.unlink(fs, abs_path);
	if (rc < 0)
		pr_err("failed to unlink path (%d)", rc);

	return rc;
}

int fs_rename(const char *from, const char *to) {
	struct fs_class *fs;
	size_t match_len;
	int rc = -EINVAL;

	if ((from == NULL) || (strlen(from) <= 1) || (from[0] != '/') || (to == NULL) ||
		(strlen(to) <= 1) || (to[0] != '/')) {
		pr_err("invalid file name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&fs, from, &match_len);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	if (fs->flags & FS_MOUNT_FLAG_READ_ONLY)
		return -EROFS;

	/* Make sure both files are mounted on the same path */
	if (strncmp(from, to, match_len) != 0) {
		pr_err("mount point not same!!");
		return -EINVAL;
	}

	rc = fs->fs_ops.rename(fs, from, to);
	if (rc < 0)
		pr_err("failed to rename file or dir (%d)", rc);

	return rc;
}

int fs_stat(const char *abs_path, struct fs_stat *stat) {
	struct fs_class *fs;
	int rc = -EINVAL;

	if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		pr_err("invalid file or dir name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&fs, abs_path, NULL);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	rc = fs->fs_ops.stat(fs, abs_path, stat);
	if (rc == -ENOENT) {
		/* File doesn't exist, which is a valid stat response */
	} else if (rc < 0) {
		pr_err("failed get file or dir stat (%d)", rc);
	}
	return rc;
}

int fs_statvfs(const char *abs_path, struct fs_statvfs *stat) {
	struct fs_class *fs;
	int rc;

	if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
		pr_err("invalid file or dir name!!");
		return -EINVAL;
	}

	rc = fs_get_mnt_point(&fs, abs_path, NULL);
	if (rc < 0) {
		pr_err("mount point not found!!");
		return rc;
	}

	rc = fs->fs_ops.statvfs(fs, abs_path, stat);
	if (rc < 0) {
		pr_err("failed get file or dir stat (%d)", rc);
	}

	return rc;
}

int fs_mount(struct fs_class *fs) {
	const struct fs_operations *fs_ops;
	struct fs_class *itr;
	struct rte_list *node;
	int rc = -EINVAL;
	size_t len = 0;

	/* Do all the mp checks prior to locking the mutex on the file
	 * subsystem.
	 */
	if ((fs->mnt_point == NULL) || (fs->storage_dev == NULL)) {
		pr_err("mount point not initialized!!");
		return -EINVAL;
	}

	/* If the link-node doest not empty */
	if (fs->node.next != NULL) {
		pr_err("file system already mounted!!");
		return -EBUSY;
	}

	len = strlen(fs->mnt_point);
	if ((len <= 1) || (fs->mnt_point[0] != '/')) {
		pr_err("invalid mount point!!");
		return -EINVAL;
	}

	tx_mutex_get(&fs_manager.mtx, TX_WAIT_FOREVER);

	/* Get file system information */
	fs_ops = fs_type_get(fs->type);
	if (fs_ops == NULL) {
		pr_err("requested file system type not registered!!");
		rc = -ENOENT;
		goto mount_err;
	}

	/* Check if mount point already exists */
	rte_list_foreach(node, &fs_manager.mnt_list) {
		itr = rte_container_of(node, struct fs_class, node);
		/* continue if length does not match */
		if (len != itr->mountp_len)
			continue;

		if (fs->fs_data == itr->fs_data) {
			pr_err("file system already mounted!!");
			rc = -EBUSY;
			goto mount_err;
		}

		if (strncmp(fs->mnt_point, itr->mnt_point, len) == 0) {
			pr_err("mount point already exists!!");
			rc = -EBUSY;
			goto mount_err;
		}
	}

	fs_operations_copy(&fs->fs_ops, fs_ops);
	rc = fs->fs_ops.mount(fs);
	if (rc < 0) {
		pr_err("fs mount error (%d)", rc);
		goto mount_err;
	}

	/* Update mount point data and append it to the list */
	fs->mountp_len = len;
	rte_list_add_tail(&fs->node, &fs_manager.mnt_list);
	pr_dbg("fs mounted at %s", fs->mnt_point);

mount_err:
	tx_mutex_put(&fs_manager.mtx);
	return rc;
}

int fs_mkfs(int fs_type, const char *dev, void *cfg, int flags) {
	int rc = -EINVAL;
	const struct fs_operations *fs_ops;

	tx_mutex_get(&fs_manager.mtx, TX_WAIT_FOREVER);

	/* Get file system information */
	fs_ops = fs_type_get(fs_type);
	if (fs_ops == NULL) {
		pr_err("fs type %d not registered!!", fs_type);
		rc = -ENOENT;
		goto mount_err;
	}

	rc = fs_ops->mkfs(dev, cfg, flags);
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

	struct fs_class *fs = fs_mounted_get(mnt_point);
	if (fs == NULL) {
		pr_err("fs not mounted (fs == %p)", fs);
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

int fs_flush(const char *mnt_point) {
	struct fs_class *fs;

	if (mnt_point == NULL)
		return -EINVAL;

	tx_mutex_get(&fs_manager.mtx, TX_WAIT_FOREVER);
	fs = fs_mounted_get(mnt_point);
	tx_mutex_put(&fs_manager.mtx);

	if (fs == NULL) {
		pr_err("fs not mounted (fs == %p)", fs);
		return -ENODATA;
	}
	
	int err = fs->fs_ops.flush(fs);
	if (err < 0)
		pr_err("fs flush error(%s)\n", err);
	return err;
}

/* Register File system */
int fs_register(int type, const struct fs_operations *fs_ops) {
	struct fs_class *iter;
	int rc = 0;

	if (fs_ops == NULL)
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

	rc = registry_add(type, fs_ops);
	pr_info("fs register %d: %d", type, rc);

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
