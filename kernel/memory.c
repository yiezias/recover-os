#include "memory.h"
#include "bitmap.h"
#include "debug.h"
#include "intr.h"
#include "print.h"
#include "tss.h"

struct {
	size_t start;
	struct bitmap pool_bitmap;
} phy_pool;

/*
 * 0x9e000~0x9f000 主线程PCB
 * 0x9d000~0x9e000 中断栈
 * 0x9c000~0x9d000 内核虚拟内存池位图
 * */

#define PHY_BITMAP_END 0xffff80000009c000

static void mem_pool_init(size_t phy_mem_size) {
	put_str("mem_pool_init: start\n");

	size_t page_cnt = phy_mem_size / PG_SIZE;
	size_t used_pages = 0x100000 / PG_SIZE + 4;
	size_t bytes_len = (page_cnt - used_pages) / 8;

	phy_pool.start = used_pages * PG_SIZE;
	phy_pool.pool_bitmap.bits = (uint8_t *)(PHY_BITMAP_END - bytes_len);
	phy_pool.pool_bitmap.bytes_len = bytes_len;

	bitmap_init(&phy_pool.pool_bitmap);

	put_info("phy_pool.start ", phy_pool.start);
	put_info("phy_pool.pool_bitmap.bits ",
		 (size_t)phy_pool.pool_bitmap.bits);
	put_info("phy_pool.pool_bitmap.bytes_len ",
		 phy_pool.pool_bitmap.bytes_len);

	put_str("mem_pool_init: done\n");
}


static size_t palloc(void) {
	ssize_t idx = bitmap_alloc(&phy_pool.pool_bitmap, 1);
	if (idx == -1) {
		PANIC("physical address allocate failed\n");
	}
	return idx * PG_SIZE + phy_pool.start;
}



struct virt_pool k_v_pool;

/* 默认虚拟内存池大小 */
#define DP_BYTES_LEN PG_SIZE
void virt_pool_init(struct virt_pool *vpool, size_t start, uint8_t *bits) {
	vpool->start = start;
	vpool->pool_bitmap.bits = bits;
	vpool->pool_bitmap.bytes_len = DP_BYTES_LEN;

	bitmap_init(&vpool->pool_bitmap);
}



enum PML {
	pt,   /* 12-20 bit */
	pdt,  /* 21-29 bit */
	pdpt, /* 30-38 bit */
	pml4, /* 39-47 bit */
};


#define PROT_P 1
#define PROT_W 2
#define PROT_U 4
#define PROT_NX 0x8000000000000000

static inline size_t page_entry_idx(size_t vaddr, enum PML pml) {
	return (vaddr >> (9 * pml + 12)) & 0x1ff;
}

static uint64_t *page_entry_ptr(size_t vaddr, enum PML pml) {
	size_t page_table_ptr = (~0xfff);
	if (pml != pml4) {
		page_table_ptr = ((size_t)(page_entry_ptr(vaddr, pml + 1)));
		page_table_ptr <<= 9;
	}
	return (uint64_t *)(page_table_ptr + (page_entry_idx(vaddr, pml) << 3));
}

static int map_status(size_t vaddr) {
	enum PML pml = pml4;
	for (; pml >= 0; --pml) {
		uint64_t *pep = page_entry_ptr(vaddr, pml);
		if (!(*pep & PROT_P)) {
			return pml;
		}
	}
	return -1;
}

static size_t addr_v2p(size_t vaddr) {
	/* 如果内存未映射可能返回不确定的值
	 * 或发生缺页中断 */
	if (map_status(vaddr) == -1) {
		return 0;
	}
	return (*page_entry_ptr(vaddr, pt) & ~0xfff) | (vaddr & 0xfff);
}


static void page_map(size_t vaddr, size_t paddr) {
	for (int i = 0; i != 3; ++i) {
		enum PML pml = 4 - i - 1;
		uint64_t *pep = page_entry_ptr(vaddr, pml);
		if (!(*pep & PROT_P)) {
			*pep = palloc() | PROT_P | PROT_W | PROT_U;
		}
	}
	uint64_t *pep = page_entry_ptr(vaddr, pt);
	*pep = paddr | PROT_P | PROT_U | PROT_W;
}

void *alloc_pages(struct virt_pool *vpool, size_t pg_cnt) {
	ssize_t idx = bitmap_alloc(&vpool->pool_bitmap, pg_cnt);
	if (idx == -1) {
		return NULL;
	}
	return (void *)(vpool->start + idx * PG_SIZE);
}

void free_pages(void *vaddr, struct virt_pool *vpool, size_t pg_cnt) {
	for (size_t i = 0; i != pg_cnt; ++i) {
		size_t vidx = ((size_t)vaddr - vpool->start) / PG_SIZE;
		size_t paddr = addr_v2p((size_t)vaddr);
		if (paddr == 0) {
			break;
		}
		size_t pidx = (paddr - phy_pool.start) / PG_SIZE;

		bitmap_set(&vpool->pool_bitmap, vidx, 0);
		bitmap_set(&phy_pool.pool_bitmap, pidx, 0);
	}
}



static struct virt_pool *get_vpool(void) {
	return &k_v_pool;
}


static inline bool in_range(size_t num, size_t start, size_t len) {
	return num >= start && num < start + len;
}

static void intr_page_handle(void) {
	size_t page_fault_vaddr;
	asm("movq %%cr2, %0" : "=r"(page_fault_vaddr));

	struct virt_pool *vpool = get_vpool();

	if (in_range(page_fault_vaddr, vpool->start,
		     vpool->pool_bitmap.bytes_len * 8 * PG_SIZE)) {
		if (bitmap_read(&vpool->pool_bitmap,
				(page_fault_vaddr - vpool->start) / PG_SIZE)) {
			goto map_valid_page;
		}
	}

invalid_page:
	UNUSED;
	put_info("\x1b\x0c\naccess address is invalid: ", page_fault_vaddr);
	while (1) {}

map_valid_page:;
	size_t paddr = palloc();
	page_map(page_fault_vaddr, paddr);
}



#define K_HEAP_START 0xffff800000100000
#define K_VP_BITMAP_START PHY_BITMAP_END

void mem_init(void) {
	put_str("mem_init: start\n");

	mem_pool_init(0x8000000);
	virt_pool_init(&k_v_pool, K_HEAP_START, (uint8_t *)K_VP_BITMAP_START);

	tss.ist1 = K_VP_BITMAP_START - 2 * PG_SIZE;
	register_intr_handle(0x0e, intr_page_handle);

	put_str("mem_init: done\n");
}
