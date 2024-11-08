#include "syscall-init.h"
#include "exec.h"
#include "file.h"
#include "print.h"
#include "syscall.h"
#include "wait_exit.h"

#define syscall_nr 32

typedef void *syscall;
syscall syscall_table[syscall_nr];

extern void system_call(void);

void syscall_init(void) {
	put_str("syscall_init: start\n");
	asm volatile("rdmsr" ::"c"(0xc0000082) :);
	asm volatile("wrmsr" ::"a"(system_call),
		     "d"((uint64_t)system_call >> 32)
		     :);
	asm volatile("rdmsr" ::"c"(0xc0000081) :);
	asm volatile("wrmsr" ::"a"(0), "d"(0x00200008) :);

	syscall_table[SYS_READ] = sys_read;
	syscall_table[SYS_WRITE] = sys_write;
	syscall_table[SYS_OPEN] = sys_open;
	syscall_table[SYS_CLOSE] = sys_close;
	syscall_table[SYS_STAT] = sys_stat;
	syscall_table[SYS_CLONE] = sys_clone;
	syscall_table[SYS_EXECV] = sys_execv;
	syscall_table[SYS_WAIT] = sys_wait;
	syscall_table[SYS_EXIT] = sys_exit;

	put_str("syscall_init: end\n");
}
