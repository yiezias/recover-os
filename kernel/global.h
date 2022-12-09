#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H
#include "types.h"

#define NULL ((void*)0)

typedef uint8_t bool;
#define false 0
#define true 1

#define SELECTOR_K_CODE 8
#define SELECTOR_K_DATA 0x10
#define SELECTOR_U_CODE (0x30 + 3)
#define SELECTOR_U_DATA (0x28 + 3)

#endif
