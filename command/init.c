#include "syscall.h"

int main(void) {
	write(1, "\x1b\x07init process\n", 15);

	pid_t pid = clone(0, 0, NULL, NULL);
	if (pid == 0) {
		const char *argv[] = { NULL };
		execv("/shell", argv);
	} else {
		int status;
		while (1) {
			wait(&status);
		}
	}

	while (1) {}
	return 0;
}
