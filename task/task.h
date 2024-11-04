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
	size_t entry;
	size_t vaddr[4];
	size_t filesz[4];
	void *segments;
	size_t segments_size;
};

typedef ssize_t pid_t;
#define MAX_FILES_OPEN_PER_PROC 32
struct task_struct {
	size_t rbp;
	uint64_t *pml4;

	pid_t pid;

	uint8_t prio;
	uint8_t ticks;

	char name[20];

	enum task_status status;

	struct list_elem general_tag;
	struct list_elem all_list_tag;

	struct task_struct *parent_task;

	struct addr_space *addr_space_ptr;
	ssize_t fd_table[MAX_FILES_OPEN_PER_PROC];
	struct intr_stack *intr_stack;

	uint64_t stack_magic;
};
#define STACK_MAGIC 0x474d575a5a46594c

struct task_struct *running_task(void);

void schedule(void);

struct task_struct *create_task(size_t stack, void *entry, void *args,
				char *name, uint8_t prio, bool su);
void task_block(enum task_status status);
void task_unblock(struct task_struct *task);
void task_yield(void);
#endif
