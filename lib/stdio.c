#include "stdio.h"
#include "global.h"
#include "string.h"
#include "syscall.h"

static void itoa(uint64_t value, char **buf, int base) {
	uint64_t m = value % base;
	uint64_t i = value / base;

	if (i) {
		itoa(i, buf, base);
	}

	if (m < 10) {
		*((*buf)++) = m + '0';
	} else {
		*((*buf)++) = m + 'A' - 10;
	}
}

static size_t vsprintf(char *str, const char *format, void *args) {
	char *wred = str;
	char idx = *format;
	int32_t arg_int;
	char *arg_str;
	int64_t arg_64;

	while (idx) {
		if (idx != '%') {
			*(str++) = idx;
			idx = *(++format);
			continue;
		}
		idx = *(++format);
		switch (idx) {
		case 'l':
			idx = *(++format);
			switch (idx) {
			case 'd':
				arg_64 = *(int64_t *)(args += 8);
				if (arg_64 < 0) {
					arg_64 = 0 - arg_64;
					*str++ = '-';
				}
				itoa(arg_64, &str, 10);
				idx = *(++format);
				break;
			case 'x':
				arg_64 = *(uint64_t *)(args += 8);
				itoa(arg_64, &str, 16);
				idx = *(++format);
				break;
			}
			break;
		case 's':
			arg_str = *(char **)(args += 8);
			strcpy(str, arg_str);
			str += strlen(arg_str);
			idx = *(++format);
			break;
		case 'c':
			*(str++) = *(char *)(args += 8);
			idx = *(++format);
			break;
		case 'd':
			arg_int = *(int *)(args += 8);
			if (arg_int < 0) {
				arg_int = 0 - arg_int;
				*str++ = '-';
			}
			itoa(arg_int, &str, 10);
			idx = *(++format);
			break;
		case 'x':
			arg_int = *(int *)(args += 8);
			itoa(arg_int, &str, 16);
			idx = *(++format);
			break;
		}
	}
	*str = 0;
	return strlen(wred);
}


/* 与编译器优化相关 */
// 仅支持5个可变参数，更多也可以实现但比较复杂
size_t printf(const char *format, ...) {
	void *args;
	asm volatile("movq %%rbp,%0" : "=g"(args)::"memory");
	args -= 0xb0;
	/* 线程不安全 */
	char *buf = (char *)brk(0);
	brk((size_t)buf + 2048);
	size_t count = vsprintf(buf, format, args);
	size_t ret = write(1, buf, count);
	brk((size_t)buf);
	return ret;
}

size_t sprintf(char *buf, const char *format, ...) {
	void *args;
	asm volatile("movq %%rbp,%0" : "=g"(args)::"memory");
	args -= 0xa8;
	vsprintf(buf, format, args);
	return strlen(buf);
}
