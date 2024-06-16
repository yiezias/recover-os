#include "task.h"
#include "debug.h"
#include "global.h"
#include "intr.h"
#include "memory.h"
#include "print.h"
#include "string.h"
#include "tss.h"

struct list all_tasks_list;
struct list ready_tasks_list;


struct task_struct *running_task(void) {
	size_t task = tss.ist2 - PG_SIZE;
	ASSERT(!(task & 0xfff));
	return (struct task_struct *)task;
}

struct task_struct *idle_task;
static void idle(void *arg UNUSED) {
	while (1) {
		task_block(TASK_BLOCKED);
		asm volatile("sti; hlt" : : : "memory");
	}
}

extern void switch_to(struct task_struct *prev, struct task_struct *next);
void schedule(void) {
	ASSERT(intr_off == get_intr_stat());
	struct task_struct *cur_task = running_task();

	if (cur_task->status == TASK_RUNNING) {
		cur_task->status = TASK_READY;

		ASSERT(cur_task->ticks == 0);
		cur_task->ticks = cur_task->prio;

		ASSERT(!elem_find(&ready_tasks_list, &cur_task->general_tag));
		list_append(&ready_tasks_list, &cur_task->general_tag);
	}
	if (list_empty(&ready_tasks_list)) {
		task_unblock(idle_task);
	}
	struct task_struct *next = elem2entry(struct task_struct, general_tag,
					      list_pop(&ready_tasks_list));

	next->status = TASK_RUNNING;
	tss.ist2 = (size_t)next + PG_SIZE;

	switch_to(cur_task, next);
}

static void init_task(struct task_struct *task, char *name, uint8_t prio) {
	strcpy(task->name, name);
	task->prio = task->ticks = prio;
	task->stack_magic = STACK_MAGIC;
	ASSERT(!elem_find(&all_tasks_list, &task->all_list_tag));
	list_append(&all_tasks_list, &task->all_list_tag);

	task->intr_stack = (struct intr_stack *)((size_t)task + PG_SIZE
						 - sizeof(struct intr_stack));
}

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

	struct task_struct *father_task = running_task();

	task_stack->rbp = (size_t) & (task->intr_stack)->rbp;
	task->intr_stack->rflags = 0x202;
	task->intr_stack->cs = father_task->intr_stack->cs;
	task->intr_stack->ss = father_task->intr_stack->ss;
	task->intr_stack->rsp = stack;
	task->intr_stack->rip = (size_t)entry;
	task->intr_stack->rdi = (size_t)args;
}

struct task_struct *create_task(size_t stack, void *entry, void *args,
				char *name, uint8_t prio) {
	struct task_struct *task = kalloc_pages(1);
	init_task(task, name, prio);
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
	init_task(main_task, "main", 30);
}

static void idle_task_init(void) {
	idle_task = create_task((size_t)kalloc_pages(1) + PG_SIZE, idle, NULL,
				"idle", 30);

	idle_task->intr_stack->cs = SELECTOR_K_CODE;
	idle_task->intr_stack->ss = SELECTOR_K_DATA;
}

void task_init(void) {
	put_str("task_init: start\n");

	list_init(&ready_tasks_list);
	list_init(&all_tasks_list);
	idle_task_init();
	make_main_task();

	put_str("task_init: end\n");
}


void task_block(enum task_status status) {
	ASSERT(status == TASK_BLOCKED);
	enum intr_stat old_stat = set_intr_stat(intr_off);
	running_task()->status = status;
	schedule();
	set_intr_stat(old_stat);
}

void task_unblock(struct task_struct *task) {
	ASSERT(task->status == TASK_BLOCKED);
	enum intr_stat old_stat = set_intr_stat(intr_off);
	ASSERT(!elem_find(&ready_tasks_list, &task->general_tag));
	list_push(&ready_tasks_list, &task->general_tag);
	set_intr_stat(old_stat);
}

void task_yield(void) {
	struct task_struct *cur = running_task();
	enum intr_stat old_stat = set_intr_stat(intr_off);

	ASSERT(!elem_find(&ready_tasks_list, &cur->general_tag));
	list_append(&ready_tasks_list, &cur->general_tag);
	cur->status = TASK_READY;
	schedule();
	set_intr_stat(old_stat);
}
