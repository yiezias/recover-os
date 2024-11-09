#ifndef __FS_FILE_H
#define __FS_FILE_H
#include "dir.h"
#include "inode.h"
#include "memory.h"

struct file {
	ssize_t f_pos;
	struct inode *f_inode;
	enum file_types f_type;
};

#define MAX_FILES_OPEN 4096
#define FILE_TABLE_SIZE (PG_SIZE / (sizeof(struct file)))

enum whence {
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
};

ssize_t get_free_slot(struct inode *inode, enum file_types f_type);
ssize_t free_fd_alloc(void);

ssize_t sys_open(const char *pathname);
ssize_t sys_close(ssize_t fd);
ssize_t sys_lseek(ssize_t fd, ssize_t offset, enum whence whence);
ssize_t sys_read(ssize_t fd, void *buf, size_t count);
ssize_t sys_mknod(const char *pathname, enum file_types type, ssize_t dev);
ssize_t sys_unlink(const char *pathname);
ssize_t sys_write(ssize_t fd, void *buf, size_t count);

extern struct file *file_table;
extern struct semaphore file_table_lock;

#define FD_INVALID 8
#define FILE_TYPE_INVALID 9

#endif
