#ifndef __USERPROG_EXEC_H
#define __USERPROG_EXEC_H
#include "task.h"
#include "types.h"

ssize_t sys_execv(char *pathname, const char *argv[]);
#endif
