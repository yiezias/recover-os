#include "init.h"
#include "intr.h"
#include "memory.h"
#include "print.h"

void init_all(void) {
	put_str("init_all: start\n");

	intr_init();
	mem_init();

	put_str("init_all: done\n");
}
