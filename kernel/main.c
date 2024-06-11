#include "init.h"
#include "print.h"

int main(void) {
	cls_screen();
	put_str("Kernel start\n");
	init_all();
	while (1) {}
	return 0;
}
