#include "timer.h"
#include "intr.h"
#include "print.h"

/* 就当无事发生 */
static void intr_time_handle(void) {}

void timer_init(void) {
	put_str("timer_init: start\n");

	register_intr_handle(0x20, intr_time_handle);

	put_str("timer_init: end\n");
}
