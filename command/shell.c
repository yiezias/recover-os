#include "syscall.h"

int main(void) {
	write(1, "here is shell\n", 14);
	return 0;
}
