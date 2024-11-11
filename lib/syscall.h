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
	SYS_BRK,
	SYS_PIPE,
	SYS_DUP2,
	SYS_CLONE,
	SYS_EXECV,
	SYS_EXIT,
	SYS_WAIT,
	SYS_MKDIR,
	SYS_RMDIR,
	SYS_UNLINK,
	SYS_MKNODE,
};
/* 从文件描述符fd中读取count个字节到buf中，返回实际读取到的字节数 */
ssize_t read(ssize_t fd, void *buf, size_t count);
/* 从buf中写入count个字节到文件描述符fd中，返回实际写入的字节数 */
ssize_t write(ssize_t fd, void *buf, size_t count);
/* 打开路径为pathname已存在的文件，返回文件描述符 */
ssize_t open(const char *pathname);
/* 关闭文件描述符为fd的文件 */
ssize_t close(ssize_t fd);
/* 获取文件大小类型等信息到stat_buf */
ssize_t stat(char *pathname, struct stat *stat_buf);
/* 创建进程，若clone_flag包含CLONE_VM，则后三个参数才会起作用，否则相当于fork */
pid_t clone(size_t clone_flag, size_t stack, void *child_fn, void *args);
/* 加载可执行文件替换原来的地址空间 */
ssize_t execv(char *pathname, const char *argv[]);
/* 等待子进程结束 */
pid_t wait(int *status);
/* 进程退出 */
void _exit(int status);
/* 创建管道文件 */
int pipe(ssize_t pipe_fd[2]);
/* 文件描述符重定向 */
ssize_t dup2(size_t oldfd, size_t newfd);
/* 改变堆空间的大小 */
ssize_t brk(size_t brk);
/* 创建文件 */
ssize_t mknod(const char *pathname, enum file_types type, ssize_t dev);
/* 删除文件 */
ssize_t unlink(const char *pathname);
/* 创建目录 */
ssize_t mkdir(char *pathname);
/* 删除目录 */
ssize_t rmdir(char *pathname);
#endif
