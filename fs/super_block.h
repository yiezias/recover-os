#ifndef __FS_SUPER_BLOCK_H
#define __FS_SUPER_BLOCK_H
#include "types.h"

#define SB_LBA 40
#define SB_MAGIC 0x595A4653

struct super_block {
	uint32_t magic;

	uint32_t inode_cnt;

	uint32_t block_bitmap_lba;
	uint32_t block_bitmap_blks;

	uint32_t inode_bitmap_lba;
	uint32_t inode_bitmap_blks;

	uint32_t inode_table_lba;
	uint32_t inode_table_blks;


	uint32_t data_start_lba;
};

#endif
