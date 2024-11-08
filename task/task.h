#ifndef __TASK_TASK_H
#define __TASK_TASK_H
#include "list.h"
#include "types.h"

void task_init(void);

struct intr_stack {
	uint64_t sregs;
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


enum task_status {
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
};

struct addr_space {
	size_t vaddr[4];
	size_t filesz[4];
};

struct switch_stack {
	uint64_t rsi;
	uint64_t rdi;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t rip;
};

typedef ssize_t pid_t;
#define MAX_FILES_OPEN_PER_PROC 32
struct task_struct {
	size_t rbp;
	uint64_t *pml4;

	pid_t pid;

	uint8_t prio;
	uint8_t ticks;

	enum task_status status;

	struct list_elem general_tag;
	struct list_elem all_list_tag;

	struct task_struct *parent_task;

	struct addr_space addr_space;
	ssize_t fd_table[MAX_FILES_OPEN_PER_PROC];
	struct intr_stack *intr_stack;
	struct switch_stack switch_stack;
	size_t stack, stack_size;

	uint64_t stack_magic;
};
#define STACK_MAGIC 0x474d575a5a46594c

#define DEFAULT_STACK 0x800000000000

struct task_struct *running_task(void);

void schedule(void);

struct task_struct *create_task(size_t stack, void *entry, void *args,
				uint8_t prio, bool su);
void task_block(enum task_status status);
void task_unblock(struct task_struct *task);
void task_yield(void);

#define CLONE_VM 0x100
#define CLONE_FILES 0x400

void copy_page(size_t d_page, size_t s_page, struct task_struct *d_task,
	       struct task_struct *s_task);
pid_t sys_clone(size_t clone_flag, size_t stack, void *child_fn, void *args);
#endif
