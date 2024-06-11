#ifndef __KERNEL_INTR_H
#define __KERNEL_INTR_H

#include "print.h"

enum intr_stat {
	intr_off,
	intr_on
};

enum intr_stat get_intr_stat(void);
enum intr_stat set_intr_stat(enum intr_stat stat);


void intr_init(void);
void register_intr_handle(int num, void *func);

static inline void intr_output(int intr_nr, uint64_t *rbp_ptr) {
	put_info("\x1b\x0c\n\nintr_nr:\t", intr_nr);

	put_info("ss_old:\t", rbp_ptr[0]);
	put_info("rsp_old:\t", rbp_ptr[-1]);
	put_info("rflags_old:\t", rbp_ptr[-2]);
	put_info("cs_old:\t\t", rbp_ptr[-3]);
	put_info("rip_old:\t", rbp_ptr[-4]);
}
#endif
