/*
 * Copyright (c) 2024 wtcat(wt1454246140@gmail.com)
 */
#ifndef SUBSYS_FS_H_
#define SUBSYS_FS_H_

#include <sys/types.h>
#include "basework/container/list.h"

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @brief File System APIs
 * @defgroup file_system_api File System APIs
 * @since 1.5
 * @version 1.0.0
 * @ingroup os_services
 * @{
 */

 #ifndef MAX_FILE_NAME
 #define MAX_FILE_NAME 255
 #endif

 #ifndef FS_PRIVATE_EXTENSION
 #define FS_PRIVATE_EXTENSION
 #endif

/* Type for fs_open flags */
typedef uint8_t fs_mode_t;
struct fs_class;
struct fs_dirent;
struct fs_statvfs;

/**
 * @addtogroup file_system_api
 * @{
 */

/**
 * @brief File object representing an open file
 *
 * The object needs to be initialized with fs_file_t_init().
 */
struct fs_file {
	/** Pointer to file object structure */
	void *filep;
	/** Pointer to mount point structure */
	struct fs_class *vfs;
	/** Open/create flags */
	fs_mode_t flags;
};

/**
 * @brief Directory object representing an open directory
 *
 * The object needs to be initialized with fs_dir_t_init().
 */
struct fs_dir {
	/** Pointer to directory object structure */
	void *dirp;
	/** Pointer to mount point structure */
	struct fs_class *vfs;
};

/**
 * @brief File System interface structure
 */
struct fs_operations {
	/**
	 * @name File operations
	 * @{
	 */
	/**
	 * Opens or creates a file, depending on flags given.
	 *
	 * @param filp File to open/create.
	 * @param fs_path Path to the file.
	 * @param flags Flags for opening/creating the file.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*open)(struct fs_file *filp, const char *fs_path,
		    fs_mode_t flags);
	/**
	 * Reads nbytes number of bytes.
	 *
	 * @param filp File to read from.
	 * @param dest Destination buffer.
	 * @param nbytes Number of bytes to read.
	 * @return Number of bytes read on success, negative errno code on fail.
	 */
	ssize_t (*read)(struct fs_file *filp, void *dest, size_t nbytes);
	/**
	 * Writes nbytes number of bytes.
	 *
	 * @param filp File to write to.
	 * @param src Source buffer.
	 * @param nbytes Number of bytes to write.
	 * @return Number of bytes written on success, negative errno code on fail.
	 */
	ssize_t (*write)(struct fs_file *filp,
					const void *src, size_t nbytes);
	/**
	 * Moves the file position to a new location in the file.
	 *
	 * @param filp File to move.
	 * @param off Relative offset from the position specified by whence.
	 * @param whence Position in the file. Possible values: SEEK_CUR, SEEK_SET, SEEK_END.
	 * @return New position in the file or negative errno code on fail.
	 */
	int (*lseek)(struct fs_file *filp, off_t off, int whence);
	/**
	 * Retrieves the current position in the file.
	 *
	 * @param filp File to get the current position from.
	 * @return Current position in the file or negative errno code on fail.
	 */
	off_t (*tell)(struct fs_file *filp);
	/**
	 * Truncates/expands the file to the new length.
	 *
	 * @param filp File to truncate/expand.
	 * @param length New length of the file.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*truncate)(struct fs_file *filp, off_t length);
	/**
	 * Flushes the cache of an open file.
	 *
	 * @param filp File to flush.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*sync)(struct fs_file *filp);
	/**
	 * Flushes the associated stream and closes the file.
	 *
	 * @param filp File to close.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*close)(struct fs_file *filp);
	/** @} */

	/**
	 * @name Directory operations
	 * @{
	 */
	/**
	 * Opens an existing directory specified by the path.
	 *
	 * @param dirp Directory to open.
	 * @param fs_path Path to the directory.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*opendir)(struct fs_dir *dirp, const char *fs_path);
	/**
	 * Reads directory entries of an open directory.
	 *
	 * @param dirp Directory to read from.
	 * @param entry Next directory entry in the dirp directory.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*readdir)(struct fs_dir *dirp, struct fs_dirent *entry);
	/**
	 * Closes an open directory.
	 *
	 * @param dirp Directory to close.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*closedir)(struct fs_dir *dirp);
	/** @} */

	/**
	 * @name File system level operations
	 * @{
	 */
	/**
	 * Mounts a file system.
	 *
	 * @param mountp Mount point.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*mount)(struct fs_class *mountp);
	/**
	 * Unmounts a file system.
	 *
	 * @param mountp Mount point.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*unmount)(struct fs_class *mountp);
	/**
	 * Deletes the specified file or directory.
	 *
	 * @param mountp Mount point.
	 * @param name Path to the file or directory to delete.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*unlink)(struct fs_class *mountp, const char *name);
	/**
	 * Renames a file or directory.
	 *
	 * @param mountp Mount point.
	 * @param from Path to the file or directory to rename.
	 * @param to New name of the file or directory.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*rename)(struct fs_class *mountp, const char *from,
					const char *to);
	/**
	 * Creates a new directory using specified path.
	 *
	 * @param mountp Mount point.
	 * @param name Path to the directory to create.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*mkdir)(struct fs_class *mountp, const char *name);
	/**
	 * Checks the status of a file or directory specified by the path.
	 *
	 * @param mountp Mount point.
	 * @param path Path to the file or directory.
	 * @param entry Directory entry.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*stat)(struct fs_class *mountp, const char *path,
					struct fs_dirent *entry);
	/**
	 * Returns the total and available space on the file system volume.
	 *
	 * @param mountp Mount point.
	 * @param path Path to the file or directory.
	 * @param stat File system statistics.
	 * @return 0 on success, negative errno code on fail.
	 */
	int (*statvfs)(struct fs_class *mountp, const char *path,
					struct fs_statvfs *stat);

	/**
	 * Formats a device to specified file system type.
	 * Available only if @kconfig{CONFIG_FILE_SYSTEM_MKFS} is enabled.
	 *
	 * @param dev_id Device identifier.
	 * @param cfg File system configuration.
	 * @param flags Formatting flags.
	 * @return 0 on success, negative errno code on fail.
	 *
	 * @note This operation destroys existing data on the target device.
	 */
	int (*mkfs)(uintptr_t dev_id, void *cfg, int flags);
};

/**
 * @brief Enumeration for directory entry types
 */
enum fs_dir_entry_type {
	/** Identifier for file entry */
	FS_DIR_ENTRY_FILE = 0,
	/** Identifier for directory entry */
	FS_DIR_ENTRY_DIR
};

/** @brief Enumeration to uniquely identify file system types.
 *
 * Zephyr supports in-tree file systems and external ones.  Each
 * requires a unique identifier used to register the file system
 * implementation and to associate a mount point with the file system
 * type.  This anonymous enum defines global identifiers for the
 * in-tree file systems.
 *
 * External file systems should be registered using unique identifiers
 * starting at @c FS_TYPE_EXTERNAL_BASE.  It is the responsibility of
 * applications that use external file systems to ensure that these
 * identifiers are unique if multiple file system implementations are
 * used by the application.
 */
enum {
	FS_EXFAT = 0,
	FS_FATFS,
	FS_LITTLEFS,
	FS_EXT2,

	/** Base identifier for external file systems. */
	FS_TYPE_EXTERNAL_BASE,
};

/** Flag prevents formatting device if requested file system not found */
#define FS_MOUNT_FLAG_NO_FORMAT (1 << 0)
/** Flag makes mounted file system read-only */
#define FS_MOUNT_FLAG_READ_ONLY (1 << 1)
/** Flag used in pre-defined mount structures that are to be mounted
 * on startup.
 *
 * This flag has no impact in user-defined mount structures.
 */
#define FS_MOUNT_FLAG_AUTOMOUNT (1 << 2)
/** Flag requests file system driver to use Disk Access API. When the flag is
 * set to the fs_class.flags prior to fs_mount call, a file system
 * needs to use the Disk Access API, otherwise mount callback for the driver
 * should return -ENOSUP; when the flag is not set the file system driver
 * should use Flash API by default, unless it only supports Disc Access API.
 * When file system will use Disk Access API and the flag is not set, the mount
 * callback for the file system should set the flag on success.
 */
#define FS_MOUNT_FLAG_USE_DISK_ACCESS (1 << 3)

/**
 * @brief File system mount info structure
 */
struct fs_class {
	/** Entry for the fs_mount_list list */
	struct rte_list node;
	/** File system type */
	int type;
	/** Mount point directory name (ex: "/fatfs") */
	const char *mnt_point;

	/* The following fields are filled by file system core */
	/** Length of Mount point string */
	size_t mountp_len;

	/** Mount flags */
	uint8_t flags;

	/** Pointer to backend storage device */
	void *storage_dev;

	/** Pointer to File system interface of the mount point */
	struct fs_operations fs_ops;

	/** Pointer to file system specific data */
	union {
		void *fs_data;
		FS_PRIVATE_EXTENSION
	};
};

/**
 * @brief Structure to receive file or directory information
 *
 * Used in functions that read the directory entries to get
 * file or directory information.
 */
struct fs_dirent {
	/**
	 * File/directory type (FS_DIR_ENTRY_FILE or FS_DIR_ENTRY_DIR)
	 */
	enum fs_dir_entry_type type;
	/** Name of file or directory */
	char name[MAX_FILE_NAME + 1];
	/** Size of file (0 if directory). */
	size_t size;
};

/**
 * @brief Structure to receive volume statistics
 *
 * Used to retrieve information about total and available space
 * in the volume.
 */
struct fs_statvfs {
	/** Optimal transfer block size */
	unsigned long f_bsize;
	/** Allocation unit size */
	unsigned long f_frsize;
	/** Size of FS in f_frsize units */
	unsigned long f_blocks;
	/** Number of free blocks */
	unsigned long f_bfree;
};


/**
 * @name fs_open open and creation mode flags
 * @{
 */
/** Open for read flag */
#define FS_O_READ       0x01
/** Open for write flag */
#define FS_O_WRITE      0x02
/** Open for read-write flag combination */
#define FS_O_RDWR       (FS_O_READ | FS_O_WRITE)
/** Bitmask for read and write flags */
#define FS_O_MODE_MASK  0x03

/** Create file if it does not exist */
#define FS_O_CREATE     0x10
/** Open/create file for append */
#define FS_O_APPEND     0x20
/** Truncate the file while opening */
#define FS_O_TRUNC      0x40
/** Bitmask for open/create flags */
#define FS_O_FLAGS_MASK 0x70


/** Bitmask for open flags */
#define FS_O_MASK       (FS_O_MODE_MASK | FS_O_FLAGS_MASK)
/**
 * @}
 */

/**
 * @name fs_seek whence parameter values
 * @{
 */
#ifndef FS_SEEK_SET
/** Seek from the beginning of file */
#define FS_SEEK_SET	0
#endif
#ifndef FS_SEEK_CUR
/** Seek from a current position */
#define FS_SEEK_CUR	1
#endif
#ifndef FS_SEEK_END
/** Seek from the end of file */
#define FS_SEEK_END	2
#endif
/**
 * @}
 */

/**
 * @brief Initialize fs_file object
 *
 * Initializes the fs_file object; the function needs to be invoked
 * on object before first use with fs_open.
 *
 * @param fp Pointer to file object
 *
 */
static inline void fs_file_init(struct fs_file *fp)
{
	fp->filep = NULL;
	fp->vfs = NULL;
	fp->flags = 0;
}

/**
 * @brief Initialize fs_dir object
 *
 * Initializes the fs_dir object; the function needs to be invoked
 * on object before first use with fs_opendir.
 *
 * @param zdp Pointer to file object
 *
 */
static inline void fs_dir_init(struct fs_dir *dirp)
{
	dirp->dirp = NULL;
	dirp->vfs = NULL;
}

/**
 * @brief Open or create file
 *
 * Opens or possibly creates a file and associates a stream with it.
 * Successfully opened file, when no longer in use, should be closed
 * with fs_close().
 *
 * @details
 * @p flags can be 0 or a binary combination of one or more of the following
 * identifiers:
 *   - @c FS_O_READ open for read
 *   - @c FS_O_WRITE open for write
 *   - @c FS_O_RDWR open for read/write (<tt>FS_O_READ | FS_O_WRITE</tt>)
 *   - @c FS_O_CREATE create file if it does not exist
 *   - @c FS_O_APPEND move to end of file before each write
 *   - @c FS_O_TRUNC truncate the file
 *
 * @warning If @p flags are set to 0 the function will open file, if it exists
 *          and is accessible, but you will have no read/write access to it.
 *
 * @param fp Pointer to a file object
 * @param file_name The name of a file to open
 * @param flags The mode flags
 *
 * @retval 0 on success;
 * @retval -EBUSY when fp is already used;
 * @retval -EINVAL when a bad file name is given;
 * @retval -EROFS when opening read-only file for write, or attempting to
 *	   create a file on a system that has been mounted with the
 *	   FS_MOUNT_FLAG_READ_ONLY flag;
 * @retval -ENOENT when the file does not exist at the path;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval -EACCES when trying to truncate a file without opening it for write.
 * @retval <0 an other negative errno code, depending on a file system back-end.
 */
int fs_open(struct fs_file *fp, const char *file_name, fs_mode_t flags);

/**
 * @brief Close file
 *
 * Flushes the associated stream and closes the file.
 *
 * @param fp Pointer to the file object
 *
 * @retval 0 on success;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval <0 a negative errno code on error.
 */
int fs_close(struct fs_file *fp);

/**
 * @brief Unlink file
 *
 * Deletes the specified file or directory
 *
 * @param path Path to the file or directory to delete
 *
 * @retval 0 on success;
 * @retval -EINVAL when a bad file name is given;
 * @retval -EROFS if file is read-only, or when file system has been mounted
 *	   with the FS_MOUNT_FLAG_READ_ONLY flag;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval <0 an other negative errno code on error.
 */
int fs_unlink(const char *path);

/**
 * @brief Rename file or directory
 *
 * Performs a rename and / or move of the specified source path to the
 * specified destination.  The source path can refer to either a file or a
 * directory.  All intermediate directories in the destination path must
 * already exist.  If the source path refers to a file, the destination path
 * must contain a full filename path, rather than just the new parent
 * directory.  If an object already exists at the specified destination path,
 * this function causes it to be unlinked prior to the rename (i.e., the
 * destination gets clobbered).
 * @note Current implementation does not allow moving files between mount
 * points.
 *
 * @param from The source path
 * @param to The destination path
 *
 * @retval 0 on success;
 * @retval -EINVAL when a bad file name is given, or when rename would cause move
 *	   between mount points;
 * @retval -EROFS if file is read-only, or when file system has been mounted
 *	   with the FS_MOUNT_FLAG_READ_ONLY flag;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval <0 an other negative errno code on error.
 */
int fs_rename(const char *from, const char *to);

/**
 * @brief Read file
 *
 * Reads up to @p size bytes of data to @p ptr pointed buffer, returns number
 * of bytes read.  A returned value may be lower than @p size if there were
 * fewer bytes available than requested.
 *
 * @param fp Pointer to the file object
 * @param ptr Pointer to the data buffer
 * @param size Number of bytes to be read
 *
 * @retval >=0 a number of bytes read, on success;
 * @retval -EBADF when invoked on fp that represents unopened/closed file;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval <0 a negative errno code on error.
 */
ssize_t fs_read(struct fs_file *fp, void *ptr, size_t size);

/**
 * @brief Write file
 *
 * Attempts to write @p size number of bytes to the specified file.
 * If a negative value is returned from the function, the file pointer has not
 * been advanced.
 * If the function returns a non-negative number that is lower than @p size,
 * the global @c errno variable should be checked for an error code,
 * as the device may have no free space for data.
 *
 * @param fp Pointer to the file object
 * @param ptr Pointer to the data buffer
 * @param size Number of bytes to be written
 *
 * @retval >=0 a number of bytes written, on success;
 * @retval -EBADF when invoked on fp that represents unopened/closed file;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval <0 an other negative errno code on error.
 */
ssize_t fs_write(struct fs_file *fp, const void *ptr, size_t size);

/**
 * @brief Seek file
 *
 * Moves the file position to a new location in the file. The @p offset is added
 * to file position based on the @p whence parameter.
 *
 * @param fp Pointer to the file object
 * @param offset Relative location to move the file pointer to
 * @param whence Relative location from where offset is to be calculated.
 * - @c FS_SEEK_SET for the beginning of the file;
 * - @c FS_SEEK_CUR for the current position;
 * - @c FS_SEEK_END for the end of the file.
 *
 * @retval 0 on success;
 * @retval -EBADF when invoked on fp that represents unopened/closed file;
 * @retval -ENOTSUP if not supported by underlying file system driver;
 * @retval <0 an other negative errno code on error.
 */
int fs_seek(struct fs_file *fp, off_t offset, int whence);

/**
 * @brief Get current file position.
 *
 * Retrieves and returns the current position in the file stream.
 *
 * @param fp Pointer to the file object
 *
 * @retval >= 0 a current position in file;
 * @retval -EBADF when invoked on fp that represents unopened/closed file;
 * @retval -ENOTSUP if not supported by underlying file system driver;
 * @retval <0 an other negative errno code on error.
 *
 * The current revision does not validate the file object.
 */
off_t fs_tell(struct fs_file *fp);

/**
 * @brief Truncate or extend an open file to a given size
 *
 * Truncates the file to the new length if it is shorter than the current
 * size of the file. Expands the file if the new length is greater than the
 * current size of the file. The expanded region would be filled with zeroes.
 *
 * @note In the case of expansion, if the volume got full during the
 * expansion process, the function will expand to the maximum possible length
 * and return success.  Caller should check if the expanded size matches the
 * requested length.
 *
 * @param fp Pointer to the file object
 * @param length New size of the file in bytes
 *
 * @retval 0 on success;
 * @retval -EBADF when invoked on fp that represents unopened/closed file;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval <0 an other negative errno code on error.
 */
int fs_truncate(struct fs_file *fp, off_t length);

/**
 * @brief Flush cached write data buffers of an open file
 *
 * The function flushes the cache of an open file; it can be invoked to ensure
 * data gets written to the storage media immediately, e.g. to avoid data loss
 * in case if power is removed unexpectedly.
 * @note Closing a file will cause caches to be flushed correctly so the
 * function need not be called when the file is being closed.
 *
 * @param fp Pointer to the file object
 *
 * @retval 0 on success;
 * @retval -EBADF when invoked on fp that represents unopened/closed file;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval <0 a negative errno code on error.
 */
int fs_sync(struct fs_file *fp);

/**
 * @brief Directory create
 *
 * Creates a new directory using specified path.
 *
 * @param path Path to the directory to create
 *
 * @retval 0 on success;
 * @retval -EEXIST if entry of given name exists;
 * @retval -EROFS if @p path is within read-only directory, or when
 *         file system has been mounted with the FS_MOUNT_FLAG_READ_ONLY flag;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval <0 an other negative errno code on error
 */
int fs_mkdir(const char *path);

/**
 * @brief Directory open
 *
 * Opens an existing directory specified by the path.
 *
 * @param zdp Pointer to the directory object
 * @param path Path to the directory to open
 *
 * @retval 0 on success;
 * @retval -EINVAL when a bad directory path is given;
 * @retval -EBUSY when zdp is already used;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval <0 a negative errno code on error.
 */
int fs_opendir(struct fs_dir *zdp, const char *path);

/**
 * @brief Directory read entry
 *
 * Reads directory entries of an open directory. In end-of-dir condition,
 * the function will return 0 and set the <tt>entry->name[0]</tt> to 0.
 *
 * @note: Most existing underlying file systems do not generate POSIX
 * special directory entries "." or "..".  For consistency the
 * abstraction layer will remove these from lower layer results so
 * higher layers see consistent results.
 *
 * @param zdp Pointer to the directory object
 * @param entry Pointer to zfs_dirent structure to read the entry into
 *
 * @retval 0 on success or end-of-dir;
 * @retval -ENOENT when no such directory found;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval <0 a negative errno code on error.
 */
int fs_readdir(struct fs_dir *zdp, struct fs_dirent *entry);

/**
 * @brief Directory close
 *
 * Closes an open directory.
 *
 * @param zdp Pointer to the directory object
 *
 * @retval 0 on success;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval <0 a negative errno code on error.
 */
int fs_closedir(struct fs_dir *zdp);

/**
 * @brief Mount filesystem
 *
 * Perform steps needed for mounting a file system like
 * calling the file system specific mount function and adding
 * the mount point to mounted file system list.
 *
 * @note Current implementation of ELM FAT driver allows only following mount
 * points: "/RAM:","/NAND:","/CF:","/SD:","/SD2:","/USB:","/USB2:","/USB3:"
 * or mount points that consist of single digit, e.g: "/0:", "/1:" and so forth.
 *
 * @param mp Pointer to the fs_class structure.  Referenced object
 *	     is not changed if the mount operation failed.
 *	     A reference is captured in the fs infrastructure if the
 *	     mount operation succeeds, and the application must not
 *	     mutate the structure contents until fs_unmount is
 *	     successfully invoked on the same pointer.
 *
 * @retval 0 on success;
 * @retval -ENOENT when file system type has not been registered;
 * @retval -ENOTSUP when not supported by underlying file system driver;
 *         when @c FS_MOUNT_FLAG_USE_DISK_ACCESS is set but driver does not
 *         support it.
 * @retval -EROFS if system requires formatting but @c FS_MOUNT_FLAG_READ_ONLY
 *	   has been set;
 * @retval <0 an other negative errno code on error.
 */
int fs_mount(const char *mnt, void *storage_dev, int type, 
    unsigned int options, void *fs_data);

/**
 * @brief Unmount filesystem
 *
 * Perform steps needed to unmount a file system like
 * calling the file system specific unmount function and removing
 * the mount point from mounted file system list.
 *
 * @param mp Pointer to the fs_class structure
 *
 * @retval 0 on success;
 * @retval -EINVAL if no system has been mounted at given mount point;
 * @retval -ENOTSUP when not supported by underlying file system driver;
 * @retval <0 an other negative errno code on error.
 */
int fs_unmount(const char *mnt_point);

/**
 * @brief Get path of mount point at index
 *
 * This function iterates through the list of mount points and returns
 * the directory name of the mount point at the given @p index.
 * On success @p index is incremented and @p name is set to the mount directory
 * name.  If a mount point with the given @p index does not exist, @p name will
 * be set to @c NULL.
 *
 * @param index Pointer to mount point index
 * @param name Pointer to pointer to path name
 *
 * @retval 0 on success;
 * @retval -ENOENT if there is no mount point with given index.
 */
int fs_readmount(int *index, const char **name);

/**
 * @brief File or directory status
 *
 * Checks the status of a file or directory specified by the @p path.
 * @note The file on a storage device may not be updated until it is closed.
 *
 * @param path Path to the file or directory
 * @param entry Pointer to the zfs_dirent structure to fill if the file or
 * directory exists.
 *
 * @retval 0 on success;
 * @retval -EINVAL when a bad directory or file name is given;
 * @retval -ENOENT when no such directory or file is found;
 * @retval -ENOTSUP when not supported by underlying file system driver;
 * @retval <0 negative errno code on error.
 */
int fs_stat(const char *path, struct fs_dirent *entry);

/**
 * @brief Retrieves statistics of the file system volume
 *
 * Returns the total and available space in the file system volume.
 *
 * @param path Path to the mounted directory
 * @param stat Pointer to the zfs_statvfs structure to receive the fs
 * statistics.
 *
 * @retval 0 on success;
 * @retval -EINVAL when a bad path to a directory, or a file, is given;
 * @retval -ENOTSUP when not implemented by underlying file system driver;
 * @retval <0 an other negative errno code on error.
 */
int fs_statvfs(const char *path, struct fs_statvfs *stat);

/**
 * @brief Create fresh file system
 *
 * @param fs_type Type of file system to create.
 * @param dev_id Id of storage device.
 * @param cfg Backend dependent init object. If NULL then default configuration is used.
 * @param flags Additional flags for file system implementation.
 *
 * @retval 0 on success;
 * @retval <0 negative errno code on error.
 */
int fs_mkfs(int fs_type, uintptr_t dev_id, void *cfg, int flags);

/**
 * @brief Register a file system
 *
 * Register file system with virtual file system.
 * Number of allowed file system types to be registered is controlled with the
 * CONFIG_FILE_SYSTEM_MAX_TYPES Kconfig option.
 *
 * @param type Type of file system (ex: @c FS_FATFS)
 * @param fs Pointer to File system
 *
 * @retval 0 on success;
 * @retval -EALREADY when a file system of a given type has already been registered;
 * @retval -ENOSCP when there is no space left, in file system registry, to add
 *	   this file system type.
 */
int fs_register(int type, struct fs_class *fs);

/**
 * @brief Unregister a file system
 *
 * Unregister file system from virtual file system.
 *
 * @param type Type of file system (ex: @c FS_FATFS)
 *
 * @retval 0 on success;
 * @retval -EINVAL when file system of a given type has not been registered.
 */
int fs_unregister(int type);

#ifdef __cplusplus
}
#endif
#endif /* SUBSYS_FS_H_ */
