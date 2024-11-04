#include "syscall.h"

void task(void);
uint8_t stack[4096];
int main(void) {
	write(1, "\x1b\x2fuser program\n", 15);
	clone(0, 0x800000000000, NULL, NULL, "/shell");

	clone(CLONE_VM | CLONE_FILES, (size_t)stack + 4096, task, NULL, NULL);
	while (1) {}
	return 0;
}

void task(void) {
	write(1, "\x1b\x1ftask different from init\n", 27);
	while (1) {}
}
