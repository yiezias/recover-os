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

#define IDT_DESC_CNT 0x30
// 中断描述符
struct idt_des {
	uint16_t off_low;
	uint16_t sct;
	uint8_t ist;
	uint8_t att;
	uint16_t off_mid;
	uint32_t off_high;
	uint32_t rsv;
} __attribute__((packed));
// packed阻止结构体对齐

struct idt_des idt_table[IDT_DESC_CNT];


void intr_init(void) {
	put_str("intr_init: start\n");
	put_str("intr_init: end\n");
}
