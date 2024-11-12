/*
 * Copyright 2024 wtcat
 */

#include <errno.h>
#include <reent.h>

#include <sys/types.h>

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
