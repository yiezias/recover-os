#ifndef __USERPROG_EXEC_H
#define __USERPROG_EXEC_H
#include "types.h"

void segment_load(ssize_t fd, size_t *segs);
ssize_t elf_parse(const char *pathname, size_t *segs);
#endif
