#include "init.h"
#include "fs.h"
#include "ide.h"
#include "intr.h"
#include "memory.h"
#include "print.h"
#include "task.h"
#include "timer.h"
#include "tss.h"

void init_all(void) {
	put_str("init_all: start\n");

	intr_init();
	timer_init();
	tss_init();
	mem_init();
	task_init();
	ide_init();
	filesys_init();

	put_str("init_all: done\n");
}
