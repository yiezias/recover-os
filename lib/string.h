#ifndef __LIB_STRING_H
#define __LIB_STRING_H

#include "types.h"

void memcpy(void *dst, const void *src, size_t size);
void memset(void *dst, uint8_t value, size_t size);
char *strcpy(char *dst, const char *src);
int strcmp(const char *a, const char *b);

#endif
