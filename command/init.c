#include "syscall.h"

void task(void);
uint8_t stack[4096];
int main(void) {
	write(1, "\x1b\x2fuser program\n", 15);

	while (1) {}
	return 0;
}

void task(void) {
	write(1, "\x1b\x1ftask different from init\n", 27);
	while (1) {}
}
