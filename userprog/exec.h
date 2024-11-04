#ifndef __USERPROG_EXEC_H
#define __USERPROG_EXEC_H
#include "types.h"
#include "task.h"

void load_addr_space(char *pathname, struct task_struct *task);
#endif
