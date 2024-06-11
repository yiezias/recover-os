#include "debug.h"
#include "init.h"
#include "print.h"

int main(void) {
	cls_screen();
	put_str("Kernel start\n");
	init_all();
	ASSERT(0);
	while (1) {}
	return 0;
}
