#include "syscall.h"

void task(void);
uint8_t stack[4096];
int main(void) {
	write(1, "\x1b\x07init process\n", 15);

	pid_t pid = clone(CLONE_FILES, 0, NULL, NULL);
	if (pid == 0) {
		const char *argv[] = { NULL };
		execv("/shell", argv);
	} else {
		char str[15] = "\x1b\x2fthe pid is 0\n";
		str[13] += pid;
		write(1, str, 15);
		char *argv[] = { NULL };
		clone(CLONE_FILES | CLONE_VM, (size_t)stack + 4096, task, argv);
	}

	while (1) {}
	return 0;
}

void task(void) {
	write(1, "\x1b\x1ftask different from init\n", 27);
	while (1) {}
}
