#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "bitmap.h"
#include "types.h"

void mem_init(void);

#define PG_SIZE 4096

/* 虚拟内存池 */
struct virt_pool {
	size_t start;
	struct bitmap pool_bitmap;
};

/* 内核虚拟内存池 */
extern struct virt_pool k_v_pool;
/* 初始化虚拟内存池 */
void virt_pool_init(struct virt_pool *vpool, size_t start, uint8_t *bits);
/* 申请内存页 */
void *alloc_pages(struct virt_pool *vpool, size_t pg_cnt);
/* 释放内存页 */
void free_pages(void *vaddr, struct virt_pool *vpool, size_t pg_cnt);

#endif
