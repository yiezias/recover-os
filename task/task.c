#include "task.h"
#include "bitmap.h"
#include "debug.h"
#include "global.h"
#include "intr.h"
#include "memory.h"
#include "print.h"
#include "string.h"
#include "sync.h"
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


uint8_t pid_bitmap_bits[128] = { 0 };
struct pid_pool {
	struct bitmap pid_bitmap;
	pid_t pid_start;
	struct semaphore pid_lock;
} pid_pool;

static void pid_pool_init(void) {
	pid_pool.pid_start = 0;
	pid_pool.pid_bitmap.bits = pid_bitmap_bits;
	pid_pool.pid_bitmap.bytes_len = 128;
	bitmap_init(&pid_pool.pid_bitmap);
	sema_init(&pid_pool.pid_lock, 1);
}

static pid_t alloc_pid(void) {
	sema_down(&pid_pool.pid_lock);
	size_t bit_idx = bitmap_alloc(&pid_pool.pid_bitmap, 1);
	sema_up(&pid_pool.pid_lock);
	return (bit_idx + pid_pool.pid_start);
}


static void init_task(struct task_struct *task, char *name, uint8_t prio) {
	task->pid = alloc_pid();
	strcpy(task->name, name);
	task->prio = task->ticks = prio;
	task->stack_magic = STACK_MAGIC;
	ASSERT(!elem_find(&all_tasks_list, &task->all_list_tag));
	list_append(&all_tasks_list, &task->all_list_tag);

	task->intr_stack = (struct intr_stack *)((size_t)task + PG_SIZE
						 - sizeof(struct intr_stack));
	for (size_t i = 0; i != MAX_FILES_OPEN_PER_PROC; ++i) {
		task->fd_table[i] = -1;
	}
	task->addr_space_ptr = NULL;
	task->stack_size = PG_SIZE;
}

#define KERNEL_PML4 ((uint64_t *)0xffff800000100000)

static void create_task_envi(struct task_struct *task, size_t stack,
			     void *entry, void *args, bool su) {
	task->rbp = (size_t)&task->switch_stack.rbp;
	extern void intr_exit(void);
	task->switch_stack.rip = (uint64_t)intr_exit;

	task->switch_stack.rbp = (size_t) & (task->intr_stack)->rbp;
	task->intr_stack->rflags = 0x202;
	if (task->pid > 1) {
		task->parent_task = running_task();
		task->pml4 = alloc_pages(1);
		memset(task->pml4, 0, PG_SIZE);
		memcpy(task->pml4 + 256, KERNEL_PML4 + 256, 2048);
	}
	if (su) {
		task->intr_stack->cs = SELECTOR_U_CODE;
		task->intr_stack->ss = SELECTOR_U_DATA;

	} else {
		task->intr_stack->cs = SELECTOR_K_CODE;
		task->intr_stack->ss = SELECTOR_K_DATA;
	}
	task->intr_stack->sregs = 0;
	task->stack = task->intr_stack->rsp = stack;
	task->intr_stack->rip = (size_t)entry;
	task->intr_stack->rdi = (size_t)args;
}

struct task_struct *create_task(size_t stack, void *entry, void *args,
				char *name, uint8_t prio, bool su) {
	struct task_struct *task = alloc_pages(1);
	init_task(task, name, prio);
	task->status = TASK_READY;
	ASSERT(!elem_find(&ready_tasks_list, &task->general_tag));
	list_append(&ready_tasks_list, &task->general_tag);

	create_task_envi(task, stack, entry, args, su);

	return task;
}

static void make_main_task(void) {
	struct task_struct *main_task = alloc_pages(1);
	put_info("main_task: ", (size_t)main_task);

	tss.ist2 = (size_t)main_task + PG_SIZE;
	put_info("ist2: ", tss.ist2);

	main_task->status = TASK_RUNNING;
	init_task(main_task, "main", 30);

	main_task->intr_stack->cs = SELECTOR_K_CODE;
	main_task->intr_stack->ss = SELECTOR_K_DATA;

	main_task->parent_task = main_task;
	main_task->pml4 = KERNEL_PML4;
}

static void idle_task_init(void) {
	idle_task = create_task((size_t)alloc_pages(1) + PG_SIZE, idle, NULL,
				"idle", 30, 0);
	idle_task->parent_task = idle_task;
	idle_task->pml4 = KERNEL_PML4;

	idle_task->intr_stack->cs = SELECTOR_K_CODE;
	idle_task->intr_stack->ss = SELECTOR_K_DATA;
}

void task_init(void) {
	put_str("task_init: start\n");

	list_init(&ready_tasks_list);
	list_init(&all_tasks_list);
	pid_pool_init();
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
