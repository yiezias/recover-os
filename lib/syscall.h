#ifndef __LIB_SYSCALL_H
#define __LIB_SYSCALL_H
#include "dir.h"
#include "task.h"
#include "types.h"

enum SYSCALL_NR {
	SYS_READ = 0,
	SYS_WRITE = 1,
	SYS_OPEN = 2,
	SYS_CLOSE = 3,
	SYS_STAT = 4,
	SYS_BRK = 12,
	SYS_PIPE = 22,
	SYS_DUP2 = 33,
	SYS_CLONE = 56,
	SYS_EXECV = 59,
	SYS_EXIT = 60,
	SYS_WAIT = 61,
	SYS_UNLINK,
	SYS_MKNODE,
};

ssize_t read(ssize_t fd, void *buf, size_t count);
ssize_t write(ssize_t fd, void *buf, size_t count);
ssize_t open(const char *pathname);
ssize_t close(ssize_t fd);
ssize_t stat(char *pathname, struct stat *stat_buf);
pid_t clone(size_t clone_flag, size_t stack, void *child_fn, void *args);
ssize_t execv(char *pathname, const char *argv[]);
pid_t wait(int *status);
void _exit(int status);
int pipe(ssize_t pipe_fd[2]);
ssize_t dup2(size_t oldfd, size_t newfd);
ssize_t brk(size_t brk);
ssize_t mknod(const char *pathname, enum file_types type, ssize_t dev);
ssize_t unlink(const char *pathname);
#endif
