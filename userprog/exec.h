#ifndef __USERPROG_EXEC_H
#define __USERPROG_EXEC_H
#include "task.h"
#include "types.h"

void load_addr_space(char *pathname, struct addr_space *addr_space_ptr);
#endif
