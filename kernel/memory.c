#include "memory.h"
#include "bitmap.h"
#include "debug.h"
#include "global.h"
#include "list.h"
#include "print.h"
#include "string.h"
#include "sync.h"
#include "tss.h"

struct mem_pool {
	size_t start;
	struct bitmap pool_bitmap;
	struct semaphore lock;
} kernel_mem_pool, phy_mem_pool;


/* 内存池虚拟地址为0xffff800000104000到0xffff8000001fffff
 * 总共1MB 252页，已经在引导中与物理地址映射完成，可以自由访问 */
static void mem_pool_init(struct mem_pool *mem_pool, size_t start,
			  size_t btmp_bytes_len, uint8_t *bits, size_t res) {
	mem_pool->start = start;
	mem_pool->pool_bitmap.bytes_len = btmp_bytes_len;
	mem_pool->pool_bitmap.bits = bits;
	bitmap_init(&mem_pool->pool_bitmap);
	sema_init(&mem_pool->lock, 1);
	for (size_t i = 0; i != res; ++i) {
		bitmap_set(&mem_pool->pool_bitmap, i, 1);
	}
}

static size_t pool_alloc(struct mem_pool *mem_pool, size_t pg_cnt) {
	sema_down(&mem_pool->lock);
	ssize_t idx = bitmap_alloc(&mem_pool->pool_bitmap, pg_cnt);
	sema_up(&mem_pool->lock);

	if (idx == -1) {
		if (mem_pool == &kernel_mem_pool) {
			put_str("\n\nkernel_pool:\n");
		} else if (mem_pool == &phy_mem_pool) {
			put_str("\n\nphy_pool:\n");
		}
		PANIC("there isn't any free page\n");
		return -1;
	}
	return (mem_pool->start + idx * PG_SIZE);
}

static void pool_free(struct mem_pool *mem_pool, size_t addr, size_t pg_cnt) {
	size_t idx_start = (size_t)(addr - mem_pool->start) / PG_SIZE;

	sema_down(&mem_pool->lock);
	for (size_t i = 0; i != pg_cnt; ++i) {
		ASSERT(bitmap_read(&mem_pool->pool_bitmap, idx_start + i));
		bitmap_set(&mem_pool->pool_bitmap, idx_start + i, 0);
	}
	sema_up(&mem_pool->lock);
}

void *alloc_pages(size_t pg_cnt) {
	return (void *)(pool_alloc(&kernel_mem_pool, pg_cnt));
}

void free_pages(void *vaddr, size_t pg_cnt) {
	pool_free(&kernel_mem_pool, (size_t)vaddr, pg_cnt);
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


#define PROT_P 1
#define PROT_W 2
#define PROT_U 4
#define PROT_NX 0x8000000000000000

#define mem_bytes_total 0x8000000
/* 物理地址空间前1M作为代码数据所需空间，第二个1M作为内核内存池，
 * 剩下126M全部映射到内核空间 */
static void map_all_phy_mem(void) {
	size_t pte_cnt = DIV_ROUND_UP(mem_bytes_total, PG_SIZE);
	size_t pdte_cnt = DIV_ROUND_UP(pte_cnt, 512);
	ASSERT(pdte_cnt < 512);

	uint64_t *pdt_virt = (uint64_t *)0xffff800000102000;
	uint64_t *pml4_virt = (uint64_t *)0xffff800000100000;
	pml4_virt[0] = 0;
	/* 本就不富裕的内存池又少了64页 */
	void *pts = alloc_pages(pdte_cnt - 1) - PG_SIZE;
	for (size_t i = 1; i != pdte_cnt; ++i) {
		uint64_t *pt = pts + i * PG_SIZE;
		pdt_virt[i] = ((uint64_t)(pt)-kernel_addr_base) | PROT_P
			      | PROT_W | PROT_U;
		for (size_t j = 0; j != 512; ++j) {
			pt[j] = (i * 0x200000 + j * PG_SIZE) | PROT_P | PROT_W
				| PROT_U;
		}
	}
}


enum PML {
	pt,   /* 12-20 bit */
	pdt,  /* 21-29 bit */
	pdpt, /* 30-38 bit */
	pml4, /* 39-47 bit */
};

// 页目录项在页目录中的位置
#define page_entry_idx(vaddr, pml) ((vaddr >> (9 * pml + 12)) & 0x1ff)
// 页目录项地址
static uint64_t *page_entry_ptr(size_t vaddr, enum PML pml) {
	// 页目录地址
	uint64_t *page_entry_page = NULL;
	if (pml != pml4) {
		uint64_t page_entry = *page_entry_ptr(vaddr, pml + 1);
		page_entry_page = (uint64_t *)(kernel_addr_base
					       + (page_entry & (~0xfff)));
	} else {
		page_entry_page = (uint64_t *)(kernel_addr_base + 0x100000);
	}
	return page_entry_page + page_entry_idx(vaddr, pml);
}


static void page_map(size_t vaddr) {
	size_t paddr = pool_alloc(&phy_mem_pool, 1);
	for (int i = 0; i != 4; ++i) {
		enum PML pml = 3 - i;
		uint64_t *pep = page_entry_ptr(vaddr, pml);
		put_info("pep\t", (size_t)pep);
		if (!(*pep & PROT_P)) {
			size_t eaddr = pml == pt ? paddr
						 : pool_alloc(&phy_mem_pool, 1);
			*pep = eaddr | PROT_P | PROT_W | PROT_U;
		}
	}
}

static void page_unmap(size_t vaddr) {
	*page_entry_ptr(vaddr, pt) &= ~PROT_P;
	asm volatile("invlpg %0" ::"m"(vaddr) : "memory");
}


void mem_init(void) {
	put_str("mem_init: start\n");

	static uint8_t kernel_mem_pool_bitmap[252 / 8] = { 0 };
	static uint8_t phy_mem_pool_bitmap[mem_bytes_total >> 12 / 8] = { 0 };

	mem_pool_init(&kernel_mem_pool, 0xffff800000104000, 252 / 8,
		      kernel_mem_pool_bitmap, 0);
	block_desc_init(k_block_descs);
	tss.ist1 = (size_t)alloc_pages(1) + PG_SIZE;

	map_all_phy_mem();
	mem_pool_init(&phy_mem_pool, 0, mem_bytes_total >> 12 / 8,
		      phy_mem_pool_bitmap, 0x200000 >> 12);

	uint8_t *vaddr = (uint8_t *)0x1000;
	page_map((size_t)vaddr);
	vaddr[0] = 0;
	page_unmap((size_t)vaddr);
	vaddr[0] = 0;
	put_str("mem_init: end\n");
}
