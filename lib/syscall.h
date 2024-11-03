#ifndef __LIB_SYSCALL_H
#define __LIB_SYSCALL_H
#include "types.h"
#include "dir.h"

enum SYSCALL_NR {
	SYS_READ,
	SYS_WRITE,
	SYS_OPEN,
	SYS_CLOSE,
	SYS_STAT,
};

ssize_t read(ssize_t fd, void *buf, size_t count);
ssize_t write(ssize_t fd, void *buf, size_t count);
ssize_t open(const char *pathname);
ssize_t close(ssize_t fd);
ssize_t stat(char *pathname, struct stat *stat_buf);
#endif
