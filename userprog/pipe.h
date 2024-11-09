#ifndef __USERPROG_PIPE_H
#define __USERPROG_PIPE_H
#include "types.h"

int sys_pipe(ssize_t pipe_fd[2]);
ssize_t sys_dup2(size_t oldfd, size_t newfd);
size_t pipe_read(size_t fd, void *buf, size_t count);
size_t pipe_write(size_t fd, const void *buf, size_t count);

#endif
