#include "task.h"
#include "debug.h"
#include "intr.h"
#include "memory.h"
#include "print.h"
#include "tss.h"

struct list all_tasks_list;
struct list ready_tasks_list;


struct task_struct *running_task(void) {
	return (struct task_struct *)(tss.ist2 & (~0xfff));
}


extern void switch_to(struct task_struct *prev, struct task_struct *next);

void schedule(void) {
	ASSERT(intr_off == get_intr_stat());
	struct task_struct *cur_task = running_task();

	if (cur_task->status == TASK_RUNNING) {
		cur_task->status = TASK_READY;
		if (cur_task->ticks == 0) {
			cur_task->ticks = cur_task->prio;
		}
		ASSERT(!elem_find(&ready_tasks_list, &cur_task->general_tag));
		list_append(&ready_tasks_list, &cur_task->general_tag);
	}
	ASSERT(!list_empty(&ready_tasks_list));

	struct task_struct *next = elem2entry(struct task_struct, general_tag,
					      list_pop(&ready_tasks_list));

	next->status = TASK_RUNNING;
	tss.ist2 = (size_t)&next->intr_stack + sizeof(struct intr_stack);

	switch_to(cur_task, next);
}


static void init_task(struct task_struct *task) {
	task->prio = task->ticks = default_prio;
	task->status = TASK_READY;
	task->stack_magic = STACK_MAGIC;

	ASSERT(!elem_find(&all_tasks_list, &task->all_list_tag));
	list_append(&all_tasks_list, &task->all_list_tag);
}

static void create_task_envirn(struct task_struct *task, void *stack,
			       void *entry, void *args) {
	struct task_stack *task_stack =
		(void *)task + PG_SIZE - sizeof(struct task_stack);
	task->rbp_ptr = &task_stack->rbp;
	extern void intr_exit(void);
	task_stack->rip = (uint64_t)intr_exit;
	task_stack->rbp = (uint64_t)&task->intr_stack.rbp;

	task->intr_stack.rdi = (uint64_t)args;
	task->intr_stack.rip = (uint64_t)entry;
	task->intr_stack.rflags = 0x202;
	if (get_intr_stat() == intr_on) {
		task->intr_stack.rsp = (size_t)task + PG_SIZE;
		task->intr_stack.ss = SELECTOR_K_DATA;
		task->intr_stack.cs = SELECTOR_K_CODE;
	} else {
		task->intr_stack.rsp = (size_t)stack;
		task->intr_stack.ss = SELECTOR_U_DATA;
		task->intr_stack.cs = SELECTOR_U_CODE;
	}
}

struct task_struct *create_task(void *stack, void *entry, void *args) {
	struct task_struct *task = alloc_pages(&k_v_pool, 1);

	init_task(task);
	create_task_envirn(task, stack, entry, args);

	ASSERT(!elem_find(&ready_tasks_list, &task->general_tag));
	list_append(&ready_tasks_list, &task->general_tag);

	return task;
}

static void make_main_task(void) {
	struct task_struct *main_task;
	asm volatile("movq %%rsp,%0;"
		     "andq $0xfffffffffffff000,%0"
		     : "=g"(main_task));
	put_info("main_task:\t", (size_t)main_task);
	tss.ist2 = (size_t)&main_task->intr_stack + sizeof(struct intr_stack);

	init_task(main_task);
	main_task->status = TASK_RUNNING;
}

void task_init(void) {
	put_str("task_init: start\n");

	list_init(&ready_tasks_list);
	list_init(&all_tasks_list);
	make_main_task();

	put_str("task_init: done\n");
}
