#include "debug.h"
#include "init.h"
#include "intr.h"
#include "print.h"

int main(void) {
	cls_screen();
	put_str("Kernel start\n");
	init_all();
	set_intr_stat(intr_on);
	while (1) {
		put_str("main\n");
	}
	return 0;
}
