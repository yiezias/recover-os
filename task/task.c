#include "task.h"
#include "print.h"
#include "tss.h"


static void make_main_task(void) {
	struct task_struct *main_task;
	asm volatile("movq %%rsp,%0;"
		     "andq $0xfffffffffffff000,%0"
		     : "=g"(main_task));
	put_info("main_task: ", (size_t)main_task);
}

void task_init(void) {
	put_str("task_init: start\n");

	make_main_task();

	put_str("task_init: end\n");
}
