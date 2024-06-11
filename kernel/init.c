#include "init.h"
#include "intr.h"

void init_all(void) {
	put_str("init_all: start\n");
	intr_init();
	put_str("init_all: end\n");
}
