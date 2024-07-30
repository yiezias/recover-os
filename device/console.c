#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "print.h"

struct console console;

void console_read(char *buf, size_t count) {
	for (size_t i = 0; i != count; ++i) {
		buf[i] = ioq_getchar(&kbd_buf);
	}
}

void console_write(const char *str, size_t count) {
	sema_down(&console.console_lock);
	for (size_t i = 0; i != count; ++i) {
		put_char(str[i]);
	}
	sema_up(&console.console_lock);
}

void console_init(void) {
	put_str("console_init: start\n");

	sema_init(&console.console_lock, 1);

	put_str("console_init: done\n");
}
