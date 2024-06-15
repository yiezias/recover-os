#ifndef __TASK_SYNC_H
#define __TASK_SYNC_H
#include "list.h"
#include "types.h"

struct semaphore {
	uint8_t value;
	struct list waiters;
};

void sema_init(struct semaphore *psema, uint8_t value);
void sema_down(struct semaphore *psema);
void sema_up(struct semaphore *psema);

#endif
