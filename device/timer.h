#ifndef __DEVICE_TIMER_H
#define __DEVICE_TIMER_H
#include "types.h"

void timer_init(void);
void mtime_sleep(size_t m_seconds);
ssize_t sys_clock_gettime(int clk_id);
#define CLOCK_MONOTONIC 1
#define CLOCK_PROCESS_CPUTIME_ID 2

#endif
