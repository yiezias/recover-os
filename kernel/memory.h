#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "types.h"

#define kernel_addr_base 0xffff800000000000

void mem_init(void);

#define PG_SIZE 4096

void *alloc_pages(size_t pg_cnt);
void free_pages(void *vaddr, size_t pg_cnt);

void *kalloc(size_t size);
void kfree(void *addr);

void page_map(size_t vaddr);
void page_unmap(size_t vaddr);

ssize_t sys_brk(size_t brk);

extern size_t global_stack_size;
#endif
