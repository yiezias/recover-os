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
void enter_the_user_mode(void);

int main(void) {
	cls_screen();
	put_str("Kernel start\n");
	init_all();

	sema_init(&sema, 1);
	create_task((size_t)alloc_pages(1) + PG_SIZE, task_a,
		    "\x1b\x0ctask_a: ", "task_a", 30, 0);
	create_task((size_t)alloc_pages(1) + PG_SIZE, task_b,
		    "\x1b\x09task_b: ", "task_b", 30, 0);
	/* 创建shell运行环境 */
	sys_clone(0, DEFAULT_STACK, NULL, NULL, "/shell");
	/* 开中断 */
	set_intr_stat(intr_on);
	enter_the_user_mode();
	while (1) {}
	return 0;
}

void enter_the_user_mode(void) {
	/* 创建init运行环境 */
	struct task_struct *main_task = running_task();
	load_addr_space("/init", &main_task->addr_space);
	/* 切换到用户态 */
	struct intr_stack *i_stack = alloc_pages(1);
	i_stack->sregs = 0;
	i_stack->cs = SELECTOR_U_CODE;
	i_stack->ss = SELECTOR_U_DATA;

	i_stack->rip = main_task->addr_space.entry;

	main_task->stack = i_stack->rsp = DEFAULT_STACK;

	i_stack->rflags = 0x202;
	asm volatile("movq %0,%%rbp;jmp intr_exit" ::"g"(&i_stack->rbp)
		     : "memory");
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
