#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H

#define SELECTOR_K_CODE 8
#define SELECTOR_K_DATA 0x10
#define SELECTOR_U_CODE (0x30 + 3)
#define SELECTOR_U_DATA (0x28 + 3)

#define UNUSED __attribute__((unused))

#define DIV_ROUND_UP(X, STEP) (((X) + (STEP)-1) / (STEP))

#endif
