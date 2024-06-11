#include "init.h"
#include "intr.h"
#include "timer.h"
#include "tss.h"

void init_all(void) {
	put_str("init_all: start\n");
	intr_init();
	tss_init();
	timer_init();
	put_str("init_all: end\n");
}
