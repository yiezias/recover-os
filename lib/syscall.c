#include "syscall.h"
#include "global.h"

#define _syscall(NUMBER)                    \
	({                                  \
		uint64_t retval;            \
		asm volatile("syscall"      \
			     : "=a"(retval) \
			     : "a"(NUMBER)  \
			     : "memory");   \
		retval;                     \
	})


ssize_t read(ssize_t fd UNUSED, void *buf UNUSED, size_t count UNUSED) {
	return _syscall(SYS_READ);
}
ssize_t write(ssize_t fd UNUSED, void *buf UNUSED, size_t count UNUSED) {
	return _syscall(SYS_WRITE);
}
