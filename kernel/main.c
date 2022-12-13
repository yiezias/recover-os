#include "debug.h"
#include "init.h"
#include "intr.h"
#include "memory.h"
#include "print.h"
#include "task.h"

void task(void *arg);

int main(void) {
	cls_screen();
	put_str("I am kernel\n");
	init_all();

	set_intr_stat(intr_on);
	create_task(NULL, task, "\x1b\x0ctask_a\t");
	create_task(NULL, task, "\x1b\x09task_b\t");
	create_task(NULL, task, "\x1b\x0atask_c\t");
	create_task(NULL, task, "\x1b\x0etask_d\t");
	while (1) {
		enum intr_stat old_stat = set_intr_stat(intr_off);
		put_str("\x1b\x07task_m\t");
		set_intr_stat(old_stat);
	}
	return 0;
}

void task(void *arg) {
	while (1) {
		enum intr_stat old_stat = set_intr_stat(intr_off);
		put_str((char *)arg);
		set_intr_stat(old_stat);
	}
}
