#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H


void panic_spin(const char *file, int line, const char *func,
		const char *condition);

#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifndef NDEBUG

#define ASSERT(CONDITION)          \
	if (!(CONDITION)) {        \
		PANIC(#CONDITION); \
	}

#else

#define ASSERT(CONDITION) ;

#endif

#endif
