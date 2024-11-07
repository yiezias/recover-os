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

void task_a(void *arg);
void task_b(void *arg);
struct semaphore sema;

void init_all(void);

int main(void) {
	cls_screen();
	put_str("Kernel start\n");
	init_all();

	sema_init(&sema, 1);
	create_task((size_t)alloc_pages(1) + PG_SIZE, task_a,
		    "\x1b\x0ctask_a: ", 30, 0);
	create_task((size_t)alloc_pages(1) + PG_SIZE, task_b,
		    "\x1b\x09task_b: ", 30, 0);
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

void task_a(void *arg) {
	void *addr = kalloc(33);
	sema_down(&sema);
	put_info(arg, (size_t)addr);
	sema_up(&sema);
	kfree(addr);
	char c = '6';
	block_modify(0, 0, &c, 1, 6);
	while (1) {}
}

void task_b(void *arg) {
	void *addr = kalloc(63);
	sema_down(&sema);
	put_info(arg, (size_t)addr);
	sema_up(&sema);
	kfree(addr);
	char c[] = { 0, '\n', 0 };
	block_read(0, 0, c, 1, 6);
	put_str(c);
	while (1) {}
}
