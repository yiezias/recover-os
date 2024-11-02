#include "syscall.h"

int init_main(void);
int init_main(void) {
	ssize_t fd = -1;
	asm volatile("syscall" : "=a"(fd) : "a"(SYS_OPEN), "D"("/dev/stdout"));
	asm volatile("syscall" ::"a"(SYS_WRITE), "D"(fd),
		     "S"("\x1b\x2fuser program\n"), "d"(14));
	asm volatile("syscall" : : "a"(SYS_CLOSE));
	while (1) {}
	return 0;
}
