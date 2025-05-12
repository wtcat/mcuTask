/*
 * Copyright 2024 wtcat
 */

#include <errno.h>
#include <reent.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>

#include <tx_api.h>
#include <service/init.h>
#include <service/printk.h>
#include <subsys/fs/fs.h>
#include <base/assert.h>

#define FILE2FD(_file) (int)((_file) - file_descriptors)
#define FD2FILE(_fd)   (file_descriptors + (_fd))

STATIC_ASSERT(SEEK_SET == FS_SEEK_SET, "");
STATIC_ASSERT(SEEK_CUR == FS_SEEK_CUR, "");
STATIC_ASSERT(SEEK_END == FS_SEEK_END, "");


struct stat;

#ifdef CONFIG_CFILE
static struct object_pool file_pool;
static struct fs_file file_descriptors[CONFIG_FILE_DESC_MAX];

static fs_mode_t to_local_fsmode(int oflag) {
    int rwflag = oflag + 1;
    fs_mode_t mode = 0;

    if ((rwflag & _FREAD) == _FREAD)
        mode |= FS_O_READ;
    if ((rwflag & _FWRITE) == _FWRITE)
        mode |= FS_O_WRITE;
    if ((oflag & O_CREAT) == O_CREAT)
        mode |= FS_O_CREATE;
    if ((oflag & O_TRUNC) == O_TRUNC)
        mode |= FS_O_TRUNC;
    if ((oflag & _FAPPEND) == _FAPPEND)
        mode |= FS_O_APPEND;

    return mode;
}

int _open_r(struct _reent *ptr, const char *path, int oflag, 
    int mode) {
    (void) ptr;
    (void) mode;
    if (path == NULL)
        return -EINVAL;
    
    struct fs_file *filp = object_allocate(&file_pool);
    if (filp == NULL)
        return -ENOMEM;

    fs_mode_t fsmode = to_local_fsmode(oflag);
    int err = fs_open(filp, path, fsmode);
    if (err) {
        object_free(&file_pool, filp);
        return err;
    }
    return FILE2FD(filp);
}

int _close_r(struct _reent *ptr, int fd) {
    struct fs_file *filp;
    int err;

    (void) ptr;
    rte_assert(fd < CONFIG_FILE_DESC_MAX);
    filp = FD2FILE(fd);
    err = fs_close(filp);
    if (!err) {
        filp->vfs = NULL;
        object_free(&file_pool, filp);
    }
    return err;
}

off_t _lseek_r(struct _reent *ptr, int fd, off_t offset, int whence) {
    rte_assert(fd < CONFIG_FILE_DESC_MAX);
    (void) ptr;
    return fs_seek(FD2FILE(fd), offset, whence);
}

_ssize_t _read_r(struct _reent *ptr, int fd, void *buf, size_t nbytes) {
    rte_assert(fd < CONFIG_FILE_DESC_MAX);
    (void) ptr;
    return fs_read(FD2FILE(fd), buf, nbytes);
}

_ssize_t _write_r(struct _reent *ptr, int fd, const void *buf, size_t nbytes) {
    rte_assert(fd < CONFIG_FILE_DESC_MAX);
    (void) ptr;
    return fs_write(FD2FILE(fd), buf, nbytes);
}

#else /* !CONFIG_CFILE */
int _open_r(struct _reent *ptr, const char *path, int oflag, 
    int mode) {
    (void) ptr;
    (void) path;
    (void) oflag;
    (void) mode;
	return -ENOSYS;
}

int _close_r(struct _reent *ptr, int fd) {
    (void) ptr;
    (void) fd;
	return -ENOSYS;
}

off_t _lseek_r(struct _reent *ptr, int fd, off_t offset, int whence) {
    (void) ptr;
    (void) fd;
    (void) offset;
    (void) whence;
	return -ENOSYS;
}

_ssize_t _read_r(struct _reent *ptr, int fd, void *buf, size_t nbytes) {
    (void) ptr;
    (void) fd;
    (void) buf;
    (void) nbytes;
	return -ENOSYS;
}

_ssize_t _write_r(struct _reent *ptr, int fd, const void *buf, size_t nbytes) {
    (void) ptr;
    (void) fd;
    (void) buf;
    (void) nbytes;
	return -ENOSYS;
}
#endif /* CONFIG_CFILE */

int _fstat(int file, struct stat *st) {
    (void) file;
    (void) st;
    return -ENOSYS;
}

int _isatty(int fd) {
    (void) fd;
    return -ENOSYS;
}

void *_sbrk(ptrdiff_t incr) {
    return NULL;
}

void _exit(int status) {
    (void) status;
    rte_assert0(0);
    for ( ; ; );
}

void __assert_func(const char *file, int line, const char *func, 
    const char *expr) {
    printk("assertion \"%s\" failed: file \"%s\", line %d%s%s\n",
	   expr, file, line,
	   func ? ", function: " : "", func ? func : "");
    while (1);
}

void __assert_failed(const char *file, int line, const char *func, 
    const char *expr) {
    printk("assertion \"%s\" failed: file \"%s\", line %d%s%s\n",
	   expr, file, line,
	   func ? ", function: " : "", func ? func : "");
    TX_SYSTEM_PANIC();
    while (1);
}

#ifdef CONFIG_CFILE
static int cstub_init(void) {
    object_pool_initialize(&file_pool, file_descriptors, 
        sizeof(file_descriptors), sizeof(file_descriptors[0]));
    return 0;
}

SYSINIT(cstub_init, SI_MEMORY_LEVEL, 10);
#endif /* CONFIG_CFILE */
