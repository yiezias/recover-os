#include "sync.h"
#include "debug.h"
#include "intr.h"
#include "task.h"


void sema_init(struct semaphore *psema, uint8_t value) {
	/* 信号量不一定只能为0或1，但在本程序当中如此 */
	ASSERT(value == 0 || value == 1);
	psema->value = value;
	list_init(&psema->waiters);
}
void sema_down(struct semaphore *psema) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	struct task_struct *current = running_task();
	while (psema->value == 0) {
		ASSERT(!elem_find(&psema->waiters, &current->general_tag));
		list_append(&psema->waiters, &current->general_tag);
		task_block(TASK_BLOCKED);
	}
	--psema->value;
	ASSERT(psema->value == 0);
	set_intr_stat(old_stat);
}

void sema_up(struct semaphore *psema) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	ASSERT(psema->value == 0);
	if (!list_empty(&psema->waiters)) {
		struct task_struct *thread_blocked =
			elem2entry(struct task_struct, general_tag,
				   list_pop(&psema->waiters));
		task_unblock(thread_blocked);
	}
	++psema->value;
	ASSERT(psema->value == 1);
	set_intr_stat(old_stat);
}
