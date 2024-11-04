#ifndef __LIB_STRING_H
#define __LIB_STRING_H

#include "types.h"

void memcpy(void *dst, const void *src, size_t size);
void memset(void *dst, uint8_t value, size_t size);
int memcmp(const void *a, const void *b, size_t size);
char *strcpy(char *dst, const char *src);
int strcmp(const char *a, const char *b);
size_t strlen(const char *str);
char *strrchr(const char *str, const char ch);
char *strchr(const char *str, const char ch);

#endif
