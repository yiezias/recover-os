#include "init.h"
#include "fs.h"
#include "ide.h"
#include "intr.h"
#include "keyboard.h"
#include "memory.h"
#include "task.h"
#include "timer.h"
#include "tss.h"

void init_all(void) {
	put_str("init_all: start\n");
	intr_init();
	tss_init();
	timer_init();
	mem_init();
	task_init();
	keyboard_init();
	ide_init();
	filesys_init();
	put_str("init_all: end\n");
}
