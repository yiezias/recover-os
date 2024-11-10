#ifndef __DEVICE_PRINT_H
#define __DEVICE_PRINT_H

#include "types.h"

void put_char(char c);
void put_str(char *str);
void put_num(uint64_t num);
void put_info(char *message, size_t num);

#endif
