#ifndef __TASK_TASK_H
#define __TASK_TASK_H
#include "list.h"
#include "types.h"

void task_init(void);

enum task_status {
	TASK_RUNNING,
	TASK_READY,
};

/* 这个值提供中断所有过程需要的栈空间
 * 不应过小，经测试最小值300左右 */
#define intr_stack_size 512
struct task_struct {
	size_t rbp;

	uint8_t prio;
	uint8_t ticks;

	enum task_status status;

	struct list_elem general_tag;
	struct list_elem all_list_tag;

	uint8_t intr_stack[intr_stack_size];

	uint64_t stack_magic;
};
#define STACK_MAGIC 0x474d575a5a46594c

struct task_struct *running_task(void);

void schedule(void);

#endif
