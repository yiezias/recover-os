#ifndef __KERNEL_INTR_H
#define __KERNEL_INTR_H

#include "print.h"
#include "task.h"

enum intr_stat {
	intr_off,
	intr_on
};

enum intr_stat get_intr_stat(void);
enum intr_stat set_intr_stat(enum intr_stat stat);


void intr_init(void);
void register_intr_handle(int num, void *func);
void put_intr_info(int intr_nr, uint64_t *rbp_ptr,
		   struct task_struct *cur_task);

#endif
