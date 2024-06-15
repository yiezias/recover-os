#include "memory.h"
#include "bitmap.h"
#include "debug.h"
#include "print.h"
#include "tss.h"

struct {
	size_t start;
	struct bitmap pool_bitmap;
} kernel_mem_pool;


/* 内存池虚拟地址为0xffff800000104000到0xffff8000001fffff
 * 总共1MB 252页，已经在引导中与物理地址映射完成，可以自由访问 */

static void kernel_mem_pool_init(void) {
	kernel_mem_pool.start = 0xffff800000104000;
	const size_t btmp_bytes_len = 252 / 8;

	static uint8_t kernel_mem_pool_bitmap[252 / 8] = { 0 };

	kernel_mem_pool.pool_bitmap.bytes_len = btmp_bytes_len;
	kernel_mem_pool.pool_bitmap.bits = kernel_mem_pool_bitmap;

	bitmap_init(&kernel_mem_pool.pool_bitmap);
}


void *kalloc_pages(size_t pg_cnt) {
	ssize_t idx = bitmap_alloc(&kernel_mem_pool.pool_bitmap, pg_cnt);
	if (idx == -1) {
		return NULL;
	}
	return (void *)(kernel_mem_pool.start + idx * PG_SIZE);
}

void kfree_pages(void *vaddr, size_t pg_cnt) {
	size_t idx_start = (size_t)(vaddr - kernel_mem_pool.start) / PG_SIZE;
	for (size_t i = 0; i != pg_cnt; ++i) {
		ASSERT(bitmap_read(&kernel_mem_pool.pool_bitmap,
				   idx_start + i));
		bitmap_set(&kernel_mem_pool.pool_bitmap, idx_start + i, 0);
	}
}

void mem_init(void) {
	put_str("mem_init: start\n");

	kernel_mem_pool_init();
	tss.ist1 = (size_t)kalloc_pages(1) + PG_SIZE;

	put_str("mem_init: end\n");
}
