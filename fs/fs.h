#ifndef __FS_FS_H
#define __FS_FS_H
#include "types.h"

#define SB_MAGIC 0x474d575a5a46594c
struct super_block {
	size_t magic;
	size_t block_bitmap_start;
	size_t block_bitmap_size;
	size_t inode_bitmap_start;
	size_t inode_table_start;
	size_t inode_cnt_max;
	size_t data_start;
};

void filesys_init(void);

#endif
