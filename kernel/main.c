#include "debug.h"
#include "init.h"
#include "intr.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "memory.h"
#include "print.h"
#include "sync.h"
#include "task.h"


int main(void) {
	cls_screen();
	put_str("Kernel start\n");
	init_all();
	set_intr_stat(intr_on);
	while (1) {
		enum intr_stat stat = set_intr_stat(intr_off);
		char byte = ioq_getchar(&kbd_buf);
		put_char(byte);
		set_intr_stat(stat);
	}
	return 0;
}
