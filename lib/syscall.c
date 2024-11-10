#include "syscall.h"
#include "global.h"

/* syscall 指令参数传递顺序rdi，rsi，rdx，r10，r8，r9 */
/* x64Linux函数参数传递顺序rdi，rsi，rdx，rcx，r8，r9 */
#define _syscall(NUMBER)                        \
	({                                      \
		uint64_t retval;                \
		asm volatile("movq %rcx,%r10"); \
		asm volatile("syscall"          \
			     : "=a"(retval)     \
			     : "a"(NUMBER)      \
			     : "memory");       \
		retval;                         \
	})


ssize_t read(ssize_t fd UNUSED, void *buf UNUSED, size_t count UNUSED) {
	return _syscall(SYS_READ);
}
ssize_t write(ssize_t fd UNUSED, void *buf UNUSED, size_t count UNUSED) {
	return _syscall(SYS_WRITE);
}
ssize_t open(const char *pathname UNUSED) {
	return _syscall(SYS_OPEN);
}
ssize_t close(ssize_t fd UNUSED) {
	return _syscall(SYS_CLOSE);
}
ssize_t stat(char *pathname UNUSED, struct stat *stat_buf UNUSED) {
	return _syscall(SYS_STAT);
}
pid_t clone(size_t clone_flag UNUSED, size_t stack UNUSED,
	    void *child_fn UNUSED, void *args UNUSED) {
	return _syscall(SYS_CLONE);
}
ssize_t execv(char *pathname UNUSED, const char *argv[] UNUSED) {
	return _syscall(SYS_EXECV);
}
pid_t wait(int *status UNUSED) {
	return _syscall(SYS_WAIT);
}
void _exit(int status UNUSED) {
	_syscall(SYS_EXIT);
}
int pipe(ssize_t pipe_fd[2] UNUSED) {
	return _syscall(SYS_PIPE);
}
ssize_t dup2(size_t oldfd UNUSED, size_t newfd UNUSED) {
	return _syscall(SYS_DUP2);
}
ssize_t brk(size_t brk UNUSED) {
	return _syscall(SYS_BRK);
}
ssize_t mknod(const char *pathname UNUSED, enum file_types type UNUSED,
	      ssize_t dev UNUSED) {
	return _syscall(SYS_MKNODE);
}
ssize_t unlink(const char *pathname UNUSED) {
	return _syscall(SYS_UNLINK);
}
