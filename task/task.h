#ifndef __TASK_TASK_H
#define __TASK_TASK_H

#include "list.h"
#include "types.h"

/* 中断栈 */
struct intr_stack {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;
	uint64_t sregs;
	uint64_t rbp;
	uint64_t err_code;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

/* 任务栈 */
struct task_stack {
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

enum task_status {
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_HANGING,
	TASK_WAITING,
	TASK_DIED,
};

#define STACK_MAGIC 0x474d575a5a46594c

typedef int pid_t;

/* 任务PCB结构 */
struct task_struct {
	uint64_t *rbp_ptr;

	pid_t pid;

	uint8_t prio;
	uint8_t ticks;

	enum task_status status;

	struct list_elem general_tag;
	struct list_elem all_list_tag;

	uint8_t extern_intr_stack[256];
	struct intr_stack intr_stack;
	uint64_t stack_magic;
};

#define default_prio 31

void task_init(void);
/* 当前运行任务 */
struct task_struct *running_task(void);
/* 创建任务 */
struct task_struct *create_task(void *stack, void *entry, void *args);
/* 切换任务 */
void schedule(void);

#endif
