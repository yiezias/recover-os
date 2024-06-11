#include "intr.h"

#define RFLAGS_IF 0x200

enum intr_stat get_intr_stat(void) {
	uint64_t rflags = 0;
	asm volatile("pushfq;popq %0" : "=g"(rflags));
	return (RFLAGS_IF & rflags) ? intr_on : intr_off;
}

enum intr_stat set_intr_stat(enum intr_stat stat) {
	enum intr_stat old_stat = get_intr_stat();
	if (old_stat ^ stat) {
		switch (stat) {
		case intr_off:
			asm volatile("cli");
			break;
		case intr_on:
			asm volatile("sti");
			break;
		}
	}

	return old_stat;
}

