#include "syscall.h"

int main(void) {
	write(1, "\x1b\x2fuser program\n", 14);
	return 0;
}
