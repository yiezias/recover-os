#include "memory.h"
#include "bitmap.h"
#include "debug.h"
#include "list.h"
#include "print.h"
#include "sync.h"
#include "tss.h"

struct {
	size_t start;
	struct bitmap pool_bitmap;
	struct semaphore lock;
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
	sema_init(&kernel_mem_pool.lock, 1);
}


void *alloc_pages(size_t pg_cnt) {
	sema_down(&kernel_mem_pool.lock);
	ssize_t idx = bitmap_alloc(&kernel_mem_pool.pool_bitmap, pg_cnt);
	sema_up(&kernel_mem_pool.lock);

	if (idx == -1) {
		PANIC("there isn't any free page\n");
		return NULL;
	}
	return (void *)(kernel_mem_pool.start + idx * PG_SIZE);
}

void free_pages(void *vaddr, size_t pg_cnt) {
	size_t idx_start = (size_t)(vaddr - kernel_mem_pool.start) / PG_SIZE;

	sema_down(&kernel_mem_pool.lock);
	for (size_t i = 0; i != pg_cnt; ++i) {
		ASSERT(bitmap_read(&kernel_mem_pool.pool_bitmap,
				   idx_start + i));
		bitmap_set(&kernel_mem_pool.pool_bitmap, idx_start + i, 0);
	}
	sema_up(&kernel_mem_pool.lock);
}

struct block {
	struct list_elem free_elem;
};

struct mem_block_desc {
	size_t block_size;
	struct list free_list;
	struct semaphore lock;
};

struct arena {
	struct mem_block_desc *desc;
	size_t free_block_cnt;
};

#define DESC_CNT 7

struct mem_block_desc k_block_descs[DESC_CNT];

static void block_desc_init(struct mem_block_desc *desc_array) {
	size_t block_size = 16;
	for (int idx = 0; idx != DESC_CNT; ++idx, block_size *= 2) {
		desc_array[idx].block_size = block_size;
		list_init(&desc_array[idx].free_list);
		sema_init(&desc_array[idx].lock, 1);
	}
}

static struct block *arena2block(struct arena *a, size_t idx) {
	return (struct block *)((size_t)(a + 1) + a->desc->block_size * idx);
}
static struct arena *block2arena(struct block *b) {
	return (struct arena *)((size_t)b & 0xfffffffffffff000);
}

static struct arena *arena_init(size_t desc_idx) {
	struct arena *a = alloc_pages(1);
	a->desc = k_block_descs + desc_idx;
	a->free_block_cnt =
		(PG_SIZE - sizeof(struct arena)) / (a->desc->block_size);
	for (size_t idx = 0; idx != a->free_block_cnt; ++idx) {
		list_push(&a->desc->free_list, &arena2block(a, idx)->free_elem);
	}

	return a;
}

void *kalloc(size_t size) {
	ASSERT(size <= 1024);
	size_t desc_idx = 0;
	for (; desc_idx != DESC_CNT; ++desc_idx) {
		if (size <= k_block_descs[desc_idx].block_size) {
			break;
		}
	}
	sema_down(&k_block_descs[desc_idx].lock);
	if (list_empty(&k_block_descs[desc_idx].free_list)) {
		arena_init(desc_idx);
	}
	struct block *b =
		elem2entry(struct block, free_elem,
			   list_pop(&k_block_descs[desc_idx].free_list));
	--block2arena(b)->free_block_cnt;
	sema_up(&k_block_descs[desc_idx].lock);
	return b;
}

void kfree(void *addr) {
	struct block *b = addr;
	struct arena *a = block2arena(b);

	sema_down(&a->desc->lock);
	ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
	list_append(&a->desc->free_list, &b->free_elem);

	size_t blocks_per_arena =
		(PG_SIZE - sizeof(struct arena)) / a->desc->block_size;
	if (++a->free_block_cnt == blocks_per_arena) {
		for (size_t idx = 0; idx != blocks_per_arena; ++idx) {
			struct block *b = arena2block(a, idx);
			ASSERT(elem_find(&a->desc->free_list, &b->free_elem));
			list_remove(&b->free_elem);
		}
		free_pages(a, 1);
	}
	sema_up(&a->desc->lock);
}

void mem_init(void) {
	put_str("mem_init: start\n");

	kernel_mem_pool_init();
	block_desc_init(k_block_descs);
	tss.ist1 = (size_t)alloc_pages(1) + PG_SIZE;

	put_str("mem_init: end\n");
}
