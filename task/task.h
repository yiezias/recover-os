#ifndef __TASK_TASK_H
#define __TASK_TASK_H
#include "types.h"

void task_init(void);

struct task_struct {
	uint64_t stack_magic;
};

#endif
