#include "syscall.h"

int main(int argc, char *argv[]);
void _start(int argc, char *argv[]);

void _start(int argc, char *argv[]) {
	ssize_t fi = open("/dev/stdin");
	ssize_t fo = open("/dev/stdout");
	main(argc, argv);
	close(fi);
	close(fo);
	_exit(0);
}
