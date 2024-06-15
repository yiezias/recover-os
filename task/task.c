#include "task.h"
#include "debug.h"
#include "global.h"
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
	list_append(&all_tasks_list, &task->all_list_tag);
}


struct intr_stack {
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rbp;
	uint64_t err_code;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

struct task_stack {
	uint64_t rbp;
	uint64_t rip;
};

static void create_task_envi(struct task_struct *task, size_t stack,
			     void *entry, void *args) {
	struct task_stack *task_stack =
		(struct task_stack *)(stack - sizeof(struct task_stack));
	task->rbp = (size_t)&task_stack->rbp;
	extern void intr_exit(void);
	task_stack->rip = (uint64_t)intr_exit;

	struct intr_stack *intr_stack =
		(struct intr_stack *)((size_t)task + PG_SIZE
				      - sizeof(struct intr_stack));
	task_stack->rbp = (size_t)&intr_stack->rbp;
	intr_stack->rflags = 0x202;
	intr_stack->cs = SELECTOR_K_CODE;
	intr_stack->ss = SELECTOR_K_DATA;
	intr_stack->rsp = stack;
	intr_stack->rip = (size_t)entry;
	intr_stack->rdi = (size_t)args;
}

struct task_struct *create_task(size_t stack, void *entry, void *args) {
	struct task_struct *task = kalloc_pages(1);
	init_task(task);
	task->status = TASK_READY;
	ASSERT(!elem_find(&ready_tasks_list, &task->general_tag));
	list_append(&ready_tasks_list, &task->general_tag);

	create_task_envi(task, stack, entry, args);

	return task;
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
