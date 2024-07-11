
#include "string.h"
#include "debug.h"

void memcpy(void *dst, const void *src, size_t size) {
	ASSERT(dst && src);
	while (size--) {
		*(char *)(dst++) = *(char *)(src++);
	}
}

void memset(void *dst, uint8_t value, size_t size) {
	ASSERT(dst);
	while (size--) {
		*(uint8_t *)dst++ = value;
	}
}

char *strcpy(char *dst, const char *src) {
	ASSERT(dst && src);
	char *ret = dst;
	while ((*dst++ = *src++)) {}
	return ret;
}

int strcmp(const char *a, const char *b) {
	ASSERT(a && b);
	while (*a != 0 && *a == *b) {
		++a;
		++b;
	}

	return *a - *b;
}
