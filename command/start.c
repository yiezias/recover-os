#include "syscall.h"

int main(int argc, char *argv[]);
void _start(int argc, char *argv[]);

void _start(int argc, char *argv[]) {
	_exit(main(argc, argv));
}
