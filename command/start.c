#include "syscall.h"

int main(int argc, char *argv[]);
void _start(int argc, char *argv[]);

void _start(int argc, char *argv[]) {
	main(argc, argv);
	_exit(0);
}
