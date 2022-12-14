#include "timer.h"
#include "debug.h"
#include "intr.h"
#include "io.h"
#include "print.h"
#include "task.h"


#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define CONTER0_VALUE INPUT_FREQUENCY / IRQ0_FREQUENCY
#define CONTER0_PORT 0x40
#define CONTER0_NO 0
#define CONTER_MODE 2
#define READ_WRITE_LATCH 3
#define PIT_CONTROL_PORT 0x43

static void freqc_set(uint8_t CounterPort, uint8_t CounterNo, uint8_t rwl,
		      uint8_t CounterMode, uint8_t CounterValue) {
	outb(PIT_CONTROL_PORT,
	     (uint8_t)(CounterNo << 6 | rwl << 4 | CounterMode << 1));
	outb(CounterPort, (uint8_t)CounterValue);
	outb(CounterPort, (uint8_t)CounterValue >> 8);
}

size_t ticks;

static void intr_time_handle(void) {
	struct task_struct *cur_task = running_task();
	ASSERT(cur_task->stack_magic == STACK_MAGIC);
	++ticks;
	if (cur_task->ticks == 0) {
		schedule();
	} else {
		--cur_task->ticks;
	}
}

void timer_init(void) {
	put_str("timer_init: start\n");
	freqc_set(CONTER0_PORT, CONTER0_NO, READ_WRITE_LATCH, CONTER_MODE,
		  (uint8_t)(CONTER0_VALUE));
	register_intr_handle(0x20, intr_time_handle);
	put_str("timer_init: done\n");
}
