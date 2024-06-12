#include "memory.h"
#include "bitmap.h"
#include "print.h"

struct {
	size_t start;
	struct bitmap pool_bitmap;
} kernel_mem_pool;


/* 内存池虚拟地址为0xffff800000100000到0xffff8000001fffff
 * 总共1MB 256页，已经在引导中与物理地址映射完成，可以自由访问 */

static void kernel_mem_pool_init(void) {
	kernel_mem_pool.start = 0xffff800000100000;
	const size_t btmp_bytes_len = 256 / 8;

	static uint8_t kernel_mem_pool_bitmap[256 / 8] = { 0 };

	kernel_mem_pool.pool_bitmap.bytes_len = btmp_bytes_len;
	kernel_mem_pool.pool_bitmap.bits = kernel_mem_pool_bitmap;

	bitmap_init(&kernel_mem_pool.pool_bitmap);
}

void mem_init(void) {
	put_str("mem_init: start\n");
	kernel_mem_pool_init();
	put_str("mem_init: end\n");
}
