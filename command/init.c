#include "syscall.h"

int main(void) {
	write(1, "\x1b\x2fuser program\n", 15);
	while (1) {}
	return 0;
}
