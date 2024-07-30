#include "bio.h"
#include "console.h"
#include "debug.h"
#include "file.h"
#include "fs.h"
#include "ide.h"
#include "intr.h"
#include "keyboard.h"
#include "memory.h"
#include "print.h"
#include "sync.h"
#include "task.h"
#include "timer.h"
#include "tss.h"

void task_a(void *arg);
void task_b(void *arg);
struct semaphore sema;

int main(void) {
	cls_screen();
	put_str("Kernel start\n");
	{
		intr_init();
		tss_init();
		timer_init();
		mem_init();
		task_init();
		keyboard_init();
		ide_init();
		filesys_init();
		console_init();
	}
	uint8_t buf[10];
	ssize_t fd = sys_open("/dev/stdin");
	ASSERT(fd >= 0);
	for (size_t i = 0; i != 10; ++i) {
		sys_read(fd, buf, 1);
		sys_write(fd, buf, 1);
	}

	set_intr_stat(intr_on);
	sema_init(&sema, 1);
	create_task((size_t)alloc_pages(1) + PG_SIZE, task_a,
		    "\x1b\x0ctask_a: ", "task_a", 30, 0);
	create_task((size_t)alloc_pages(1) + PG_SIZE, task_b,
		    "\x1b\x09task_b: ", "task_b", 30, 0);
	while (1) {}
	return 0;
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
	char c = 0;
	block_read(0, 0, &c, 1, 6);
	put_char(c);
	while (1) {}
}
