#include "debug.h"
#include "ide.h"
#include "init.h"
#include "intr.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "memory.h"
#include "print.h"
#include "string.h"
#include "sync.h"
#include "task.h"


int main(void) {
	cls_screen();
	put_str("Kernel start\n");
	init_all();
	set_intr_stat(intr_on);
	while (1) {}
	return 0;
}
