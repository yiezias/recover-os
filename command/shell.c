#include "syscall.h"

int main(void) {
	write(1, "\x1b\x4fhere is shell\n", 16);
	return 0;
}
