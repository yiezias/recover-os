#include "task.h"
#include "debug.h"
#include "print.h"
#include "tss.h"

struct list all_tasks_list;
struct list ready_tasks_list;


struct task_struct *running_task(void) {
	return (struct task_struct *)(tss.ist2 & (~0xfff));
}

#define default_prio 30
#define STACK_MAGIC 0x474d575a5a46594c
static void init_task(struct task_struct *task) {
	task->prio = task->ticks = default_prio;
	task->stack_magic = STACK_MAGIC;
	ASSERT(!elem_find(&all_tasks_list, &task->all_list_tag));
}


static void make_main_task(void) {
	struct task_struct *main_task;
	asm volatile("movq %%rsp,%0;"
		     "andq $0xfffffffffffff000,%0"
		     : "=g"(main_task));
	put_info("main_task: ", (size_t)main_task);

	main_task->status = TASK_RUNNING;
	init_task(main_task);
}

void task_init(void) {
	put_str("task_init: start\n");

	list_init(&ready_tasks_list);
	list_init(&all_tasks_list);
	make_main_task();

	put_str("task_init: end\n");
}
