#include "memory.h"
#include "print.h"

/* 内存池虚拟地址为0xffff800000100000到0xffff8000001fffff
 * 总共1MB 256页，已经在引导中与物理地址映射完成，可以自由访问 */

void mem_init(void) {
	put_str("mem_init: start\n");
	put_str("mem_init: end\n");
}
