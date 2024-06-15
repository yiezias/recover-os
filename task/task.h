#ifndef __TASK_TASK_H
#define __TASK_TASK_H
#include "list.h"
#include "types.h"

void task_init(void);

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


enum task_status {
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
};

struct task_struct {
	size_t rbp;

	uint8_t prio;
	uint8_t ticks;

	enum task_status status;

	struct list_elem general_tag;
	struct list_elem all_list_tag;

	struct intr_stack *intr_stack;

	uint64_t stack_magic;
};
#define STACK_MAGIC 0x474d575a5a46594c

struct task_struct *running_task(void);

void schedule(void);

struct task_struct *create_task(size_t stack, void *entry, void *args);
void task_block(enum task_status status);
void task_unblock(struct task_struct *task);
void task_yield(void);
#endif
