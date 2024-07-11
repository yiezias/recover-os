#ifndef __FS_INODE_H
#define __FS_INODE_H
#include "list.h"
#include "sync.h"
#include "types.h"

#define INODE_CNT_MAX 128
#define DISK_INODE_SIZE 128

#define NDIRECT 12

struct disk_inode {
	size_t file_size;
	/* 简单起见，间接块就不设置了，文件最大值为48K */
	size_t direct[NDIRECT];
};

struct inode {
	size_t i_no;
	struct disk_inode disk_inode;
	size_t open_cnts;
	struct list_elem inode_tag;
	struct semaphore inode_lock;
};

extern struct list open_inodes;
extern struct semaphore open_inodes_lock;
extern struct semaphore fs_bitmap_lock;

struct inode *inode_open(size_t i_no);
void inode_close(struct inode *inode);
ssize_t inode_size_change(struct inode *inode, size_t size);
ssize_t inode_write(struct inode *inode, const void *buf, size_t nbyte,
		    size_t offset);
ssize_t inode_read(struct inode *inode, void *buf, size_t nbyte, size_t offset);
ssize_t disk_inode_create(void);
void disk_inode_delete(size_t i_no);

#define NO_BIT_IN_FS_BTMP 1
#define FILE_SIZE_TOO_BIG 2
#endif
