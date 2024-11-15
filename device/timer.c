#include "timer.h"
#include "debug.h"
#include "global.h"
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

size_t ticks = 0;

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

	put_str("timer_init: end\n");
}

#define mil_seconds_per_intr (1000 / IRQ0_FREQUENCY)
static void ticks_to_sleep(size_t sleep_ticks) {
	size_t start_tick = ticks;
	while (ticks - start_tick < sleep_ticks) {
		task_yield();
	}
}

void mtime_sleep(size_t m_seconds) {
	size_t sleep_ticks = DIV_ROUND_UP(m_seconds, mil_seconds_per_intr);
	ASSERT(sleep_ticks > 0);
	ticks_to_sleep(sleep_ticks);
}

ssize_t sys_clock_gettime(int clk_id) {
	struct task_struct *cur_task = running_task();
	switch (clk_id) {
	case CLOCK_MONOTONIC:
		return ticks * mil_seconds_per_intr;
		break;
	case CLOCK_PROCESS_CPUTIME_ID:
		return cur_task->ticks * mil_seconds_per_intr;
		break;
	}
	return -1;
}
