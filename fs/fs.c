#include "fs.h"
#include "bio.h"
#include "debug.h"
#include "ide.h"
#include "memory.h"
#include "print.h"

struct partition_table_entry {
	uint8_t bootable;
	uint8_t start_head;
	uint8_t start_sec;
	uint8_t start_chs;
	uint8_t fs_type;
	uint8_t end_head;
	uint8_t end_sec;
	uint8_t end_chs;

	uint32_t start_lba;
	uint32_t sec_cnt;
} __attribute__((packed));

struct boot_sector {
	uint8_t code[446];
	struct partition_table_entry partition_table[4];
	uint16_t signature;
} __attribute__((packed));

struct boot_sector mbr;
struct partition_table_entry *cur_part;

static void mbr_init(void) {
	ide_read(0, 0, &mbr, 1);
	cur_part = mbr.partition_table;
	for (int i = 0; i != 4; ++i) {
		if (cur_part[i].bootable == 0x80) {
			cur_part += i;
			return;
		}
	}
	PANIC("There isn't any bootable partition\n");
}


struct super_block *sb;
static void partition_format(void) {
	//	sb->magic = SB_MAGIC;
	size_t super_block_bid = cur_part->start_lba / SECTS_PER_BLOCK;
	size_t block_cnt = cur_part->start_sec / SECTS_PER_BLOCK;
	sb->block_bitmap_start = super_block_bid + 1;
	sb->block_bitmap_size = block_cnt;
}
void filesys_init(void) {
	put_str("filesys_init: start\n");

	mbr_init();

	sb = alloc_pages(1);
	block_read(0, cur_part->start_lba / SECTS_PER_BLOCK, sb, BLOCK_SIZE, 0);
	if (sb->magic != SB_MAGIC) {
		partition_format();
	}

	put_str("filesys_init: end\n");
}
