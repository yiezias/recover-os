#include "debug.h"
#include "init.h"
#include "intr.h"
#include "memory.h"
#include "print.h"
#include "task.h"
void func(char *str);

int main(void) {
	cls_screen();
	put_str("Kernel start\n");
	init_all();
	set_intr_stat(intr_on);
	create_task((size_t)kalloc_pages(1) + PG_SIZE, func, "\x1b\x0ctask\n");
	create_task((size_t)kalloc_pages(1) + PG_SIZE, func, "\x1b\x0atask\n");
	create_task((size_t)kalloc_pages(1) + PG_SIZE, func, "\x1b\x0etask\n");
	while (1) {
		enum intr_stat old = set_intr_stat(intr_off);
		put_str("\x1b\x09main\n");
		set_intr_stat(old);
	}
	return 0;
}

void func(char *str) {
	while (1) {
		enum intr_stat old = set_intr_stat(intr_off);
		put_str(str);
		set_intr_stat(old);
	}
}
