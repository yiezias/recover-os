#include "init.h"
#include "intr.h"
#include "memory.h"
#include "print.h"
#include "sync.h"
#include "task.h"

void task_a(void *arg);
void task_b(void *arg);
struct semaphore sema;

int main(void) {
	cls_screen();
	put_str("Kernel start\n");
	init_all();
	set_intr_stat(intr_on);
	sema_init(&sema, 1);
	create_task((size_t)kalloc_pages(1) + PG_SIZE, task_a,
		    "\x1b\x0ctask_a: ", "task_a", 30);
	create_task((size_t)kalloc_pages(1) + PG_SIZE, task_b,
		    "\x1b\x09task_b: ", "task_b", 30);
	while (1) {}
	return 0;
}

void task_a(void *arg) {
	void *addr = kalloc(33);
	sema_down(&sema);
	put_info(arg, (size_t)addr);
	sema_up(&sema);
	while (1) {}
}

void task_b(void *arg) {
	void *addr = kalloc(63);
	sema_down(&sema);
	put_info(arg, (size_t)addr);
	sema_up(&sema);
	while (1) {}
}
