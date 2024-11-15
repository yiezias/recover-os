#include "syscall-init.h"
#include "exec.h"
#include "file.h"
#include "pipe.h"
#include "print.h"
#include "syscall.h"
#include "timer.h"
#include "wait_exit.h"

#define syscall_nr 64

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
	syscall_table[SYS_PIPE] = sys_pipe;
	syscall_table[SYS_DUP2] = sys_dup2;
	syscall_table[SYS_BRK] = sys_brk;
	syscall_table[SYS_MKNODE] = sys_mknod;
	syscall_table[SYS_UNLINK] = sys_unlink;
	syscall_table[SYS_MKDIR] = sys_mkdir;
	syscall_table[SYS_RMDIR] = sys_rmdir;
	syscall_table[SYS_CLOCK_GETTIME] = sys_clock_gettime;
	syscall_table[SYS_SCHED_YIELD] = task_yield;
	//	syscall_table[SYS_TRIAL] = ;

	put_str("syscall_init: end\n");
}
