#ifndef __FS_INODE_H
#define __FS_INODE_H
#include "types.h"

#define INODE_CNT_MAX 128
#define DISK_INODE_SIZE 128

#define NDIRECT 12

struct disk_inode {
	size_t file_size;
	/* 简单起见，间接块就不设置了，文件最大值为48K */
	size_t direct[NDIRECT];
};

#endif
