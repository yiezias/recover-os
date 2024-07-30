#ifndef __DEVICE_CONSOLE_H
#define __DEVICE_CONSOLE_H
#include "sync.h"

struct console {
	struct semaphore console_lock;
};

void console_init(void);
void console_write(const char *str, size_t count);
void console_read(char *buf, size_t count);

extern struct console console;
#endif
