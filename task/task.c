#include "task.h"
#include "bitmap.h"
#include "debug.h"
#include "exec.h"
#include "file.h"
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


static void init_task(struct task_struct *task, uint8_t prio) {
	task->pid = alloc_pid();
	task->parent_task = task->pid > 1 ? running_task() : task;
	task->pml4 = task->parent_task->pml4;
	memset(&task->addr_space, 0, sizeof(struct addr_space));
	task->prio = task->ticks = prio;
	task->stack_magic = STACK_MAGIC;
	ASSERT(!elem_find(&all_tasks_list, &task->all_list_tag));
	list_append(&all_tasks_list, &task->all_list_tag);

	task->intr_stack = (struct intr_stack *)((size_t)task + PG_SIZE
						 - sizeof(struct intr_stack));
	for (size_t i = 0; i != MAX_FILES_OPEN_PER_PROC; ++i) {
		task->fd_table[i] = -1;
	}
	task->stack_size =
		task->pid > 1 ? task->parent_task->stack_size : PG_SIZE;
}

#define KERNEL_PML4 ((uint64_t *)0xffff800000100000)

static void create_task_envi(struct task_struct *task, size_t stack,
			     void *entry, void *args, bool su) {
	task->rbp = (size_t)&task->switch_stack.rbp;
	extern void intr_exit(void);
	task->switch_stack.rip = (uint64_t)intr_exit;

	task->switch_stack.rbp = (size_t) & (task->intr_stack)->rbp;
	task->intr_stack->rflags = 0x202;
	task->intr_stack->cs = su ? SELECTOR_U_CODE : SELECTOR_K_CODE;
	task->intr_stack->ss = su ? SELECTOR_U_DATA : SELECTOR_K_DATA;
	task->intr_stack->sregs = 0;
	task->stack = task->intr_stack->rsp = stack;
	task->intr_stack->rip = (size_t)entry;
	task->intr_stack->rdi = (size_t)args;
}

struct task_struct *create_task(size_t stack, void *entry, void *args,
				uint8_t prio, bool su) {
	struct task_struct *task = alloc_pages(1);
	init_task(task, prio);
	task->status = TASK_READY;
	ASSERT(!elem_find(&ready_tasks_list, &task->general_tag));
	list_append(&ready_tasks_list, &task->general_tag);

	create_task_envi(task, stack, entry, args, su);

	return task;
}

struct task_struct *init_proc;
static void make_main_task(void) {
	struct task_struct *main_task = alloc_pages(1);
	put_info("main_task: ", (size_t)main_task);

	tss.ist2 = (size_t)main_task + PG_SIZE;
	put_info("ist2: ", tss.ist2);

	main_task->status = TASK_RUNNING;
	init_task(main_task, 30);

	main_task->intr_stack->cs = SELECTOR_K_CODE;
	main_task->intr_stack->ss = SELECTOR_K_DATA;

	main_task->parent_task = main_task;
	main_task->pml4 = KERNEL_PML4;

	init_proc = main_task;
}

static void idle_task_init(void) {
	idle_task = create_task((size_t)alloc_pages(1) + PG_SIZE, idle, NULL,
				30, 0);
	idle_task->parent_task = idle_task;
	idle_task->pml4 = KERNEL_PML4;

	idle_task->intr_stack->cs = SELECTOR_K_CODE;
	idle_task->intr_stack->ss = SELECTOR_K_DATA;
}

void copy_page(size_t d_page, size_t s_page, struct task_struct *d_task,
	       struct task_struct *s_task) {
	uint64_t *pml4 = running_task()->pml4;
	void *page_buf = alloc_pages(1);

	asm volatile(
		"movq %0,%%cr3" ::"r"((size_t)s_task->pml4 - kernel_addr_base));
	memcpy(page_buf, (void *)s_page, PG_SIZE);

	asm volatile(
		"movq %0,%%cr3" ::"r"((size_t)d_task->pml4 - kernel_addr_base));
	page_map(d_page);
	memcpy((void *)d_page, page_buf, PG_SIZE);

	asm volatile("movq %0,%%cr3" ::"r"((size_t)pml4 - kernel_addr_base));
	free_pages(page_buf, 1);
}

extern void system_ret(void);
size_t rsp_buf;
pid_t sys_clone(size_t clone_flag, size_t stack, void *child_fn, void *args) {
	uint64_t syscall_rbp;
	asm volatile("movq (%%rbp),%0" : "=a"(syscall_rbp));

	bool su = 1;
	if (!(clone_flag & CLONE_VM)) {
		stack = running_task()->stack;
		child_fn = system_ret;
		args = NULL;
		su = 0;
	}
	struct task_struct *task = create_task(stack, child_fn, args, 30, su);
	if (!(clone_flag & CLONE_VM)) {
		// 修改返回值
		*(uint64_t *)(syscall_rbp - 8) = 0;

		task->intr_stack->rbp = syscall_rbp;

		/* 创建分页 */
		task->pml4 = alloc_pages(1);
		memset(task->pml4, 0, PG_SIZE);
		memcpy(task->pml4 + 256, task->parent_task->pml4 + 256,
		       PG_SIZE / 2);
		memcpy(&task->addr_space, &task->parent_task->addr_space,
		       sizeof(struct addr_space));
		/* 复制栈 */
		ASSERT(task->parent_task->stack_size == PG_SIZE);
		size_t d_page = task->stack - task->stack_size;
		size_t s_page = task->parent_task->stack
				- task->parent_task->stack_size;

		void *tmp_stack = alloc_pages(1);
		asm volatile("movq %%rsp,%0" : "=g"(rsp_buf)::"memory");
		asm volatile("movq %0,%%rsp" ::"g"(tmp_stack + PG_SIZE));
		copy_page(d_page, s_page, task, task->parent_task);
		asm volatile("movq %0,%%rsp" ::"g"(rsp_buf));
		free_pages(tmp_stack, 1);
	}

	if (clone_flag & CLONE_FILES) {
		memcpy(task->fd_table, task->parent_task->fd_table,
		       MAX_FILES_OPEN_PER_PROC * 8);
		for (size_t i = 0; i != MAX_FILES_OPEN_PER_PROC; ++i) {
			ssize_t ft_idx = task->fd_table[i];
			if (ft_idx != -1) {
				struct file *pf = file_table + ft_idx;
				if (FT_FIFO == pf->f_type) {
					++pf->f_pos;
				} else {
					++pf->f_inode->open_cnts;
				}
			}
		}
	}
	return task->pid;
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
	ASSERT(status == TASK_BLOCKED || status == TASK_HANGING
	       || status == TASK_WAITING);
	enum intr_stat old_stat = set_intr_stat(intr_off);
	running_task()->status = status;
	schedule();
	set_intr_stat(old_stat);
}

void task_unblock(struct task_struct *task) {
	ASSERT(task->status == TASK_BLOCKED || task->status == TASK_HANGING
	       || task->status == TASK_WAITING);
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
