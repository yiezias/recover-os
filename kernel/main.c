#include "debug.h"
#include "init.h"
#include "intr.h"
#include "print.h"

int main(void) {
	cls_screen();
	put_str("I'am kernel\n");
	init_all();
	while (1) {}
	return 0;
}
