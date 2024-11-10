
#include "assert.h"
#include "stdio.h"
#include "syscall.h"

void user_spin(const char *file, int line, const char *func,
	       const char *condition) {
	printf("\n\n\n\x1b\x04filename %s\nline %d\nfunction %s\ncondition "
	       "%s\n\x1b\x07",
	       file, line, func, condition);
	_exit(134);
}
