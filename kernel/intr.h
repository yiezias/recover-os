#ifndef __KERNEL_INTR_H
#define __KERNEL_INTR_H

#include "print.h"

enum intr_stat {
	intr_off,
	intr_on
};

enum intr_stat get_intr_stat(void);
enum intr_stat set_intr_stat(enum intr_stat stat);

#endif
