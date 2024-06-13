#include "task.h"
#include "debug.h"
#include "intr.h"
#include "memory.h"
#include "print.h"
#include "tss.h"

struct list all_tasks_list;
struct list ready_tasks_list;


struct task_struct *running_task(void) {
	size_t task = tss.ist2 - PG_SIZE;
	ASSERT(!(task & 0xfff));
	return (struct task_struct *)task;
}

extern void switch_to(struct task_struct *prev, struct task_struct *next);
void schedule(void) {
	ASSERT(intr_off == get_intr_stat());
	struct task_struct *cur_task = running_task();

	ASSERT(cur_task->status == TASK_RUNNING);
	cur_task->status = TASK_READY;

	ASSERT(cur_task->ticks == 0);
	cur_task->ticks = cur_task->prio;

	ASSERT(!elem_find(&ready_tasks_list, &cur_task->general_tag));
	list_append(&ready_tasks_list, &cur_task->general_tag);

	ASSERT(!list_empty(&ready_tasks_list));
	struct task_struct *next = elem2entry(struct task_struct, general_tag,
					      list_pop(&ready_tasks_list));

	next->status = TASK_RUNNING;
	tss.ist2 = (size_t)next + PG_SIZE;

	switch_to(cur_task, next);
}

#define default_prio 30
static void init_task(struct task_struct *task) {
	task->prio = task->ticks = default_prio;
	task->stack_magic = STACK_MAGIC;
	ASSERT(!elem_find(&all_tasks_list, &task->all_list_tag));
}


static void make_main_task(void) {
	struct task_struct *main_task = kalloc_pages(1);
	put_info("main_task: ", (size_t)main_task);

	tss.ist2 = (size_t)main_task + PG_SIZE;
	put_info("ist2: ", tss.ist2);

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
