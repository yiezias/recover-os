#include "bio.h"
#include "console.h"
#include "debug.h"
#include "exec.h"
#include "file.h"
#include "fs.h"
#include "global.h"
#include "ide.h"
#include "intr.h"
#include "keyboard.h"
#include "memory.h"
#include "print.h"
#include "string.h"
#include "sync.h"
#include "syscall-init.h"
#include "task.h"
#include "timer.h"
#include "tss.h"

void init_all(void);

int main(void) {
	put_char('\f');
	put_str("Kernel start\n");
	init_all();

	/* 开中断 */
	set_intr_stat(intr_on);

	const char *argv[] = { NULL };

	struct task_struct *main_task = running_task();
	main_task->stack = DEFAULT_STACK;
	uint64_t *rbp = (uint64_t *)(DEFAULT_STACK - 8);
	page_map(DEFAULT_STACK - PG_SIZE);
	rbp[-3] = 0x202;
	asm volatile("movq %0,%%rbp" ::"g"(rbp));

	sys_execv("/init", argv);
	while (1) {}
	return 0;
}

void init_all(void) {
	intr_init();
	tss_init();
	timer_init();
	mem_init();
	task_init();
	keyboard_init();
	ide_init();
	filesys_init();
	console_init();
	syscall_init();
}
