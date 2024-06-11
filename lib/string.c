
#include "string.h"

void memcpy(void *dst, const void *src, size_t size) {
	while (size--) {
		*(char *)(dst++) = *(char *)(src++);
	}
}

void memset(void *dst, uint8_t value, size_t size) {
	while (size--) {
		*(uint8_t *)dst++ = value;
	}
}

char *strcpy(char *dst, const char *src) {
	char *ret = dst;
	while ((*dst++ = *src++)) {}
	return ret;
}
