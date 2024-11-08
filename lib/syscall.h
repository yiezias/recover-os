#ifndef __LIB_SYSCALL_H
#define __LIB_SYSCALL_H
#include "dir.h"
#include "task.h"
#include "types.h"

enum SYSCALL_NR {
	SYS_READ,
	SYS_WRITE,
	SYS_OPEN,
	SYS_CLOSE,
	SYS_STAT,
	SYS_PIPE,
	SYS_DUP2,
	SYS_CLONE,
	SYS_EXECV,
	SYS_EXIT,
	SYS_WAIT,
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
#endif
