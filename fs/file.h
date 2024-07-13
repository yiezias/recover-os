#ifndef __FS_FILE_H
#define __FS_FILE_H
#include "inode.h"
#include "memory.h"

enum file_mode {
	readable = 1,
	writable = 1 << 1,
};

struct file {
	ssize_t f_pos;
	struct inode *f_inode;
	enum file_mode f_mode;
};

#define MAX_FILES_OPEN 4096
#define FILE_TABLE_SIZE (PG_SIZE / (sizeof(struct file)))

ssize_t sys_open(const char *pathname);
ssize_t sys_close(ssize_t fd);

extern struct file *file_table;
extern struct semaphore file_table_lock;

#define FD_INVALID 8

#endif
