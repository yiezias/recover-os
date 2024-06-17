#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "types.h"

void mem_init(void);

#define PG_SIZE 4096

void *kalloc_pages(size_t pg_cnt);
void kfree_pages(void *vaddr, size_t pg_cnt);

void *kalloc(size_t size);
void kfree(void *addr);
#endif
