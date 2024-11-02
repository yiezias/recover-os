#include "fs.h"
#include "bio.h"
#include "debug.h"
#include "dir.h"
#include "file.h"
#include "global.h"
#include "ide.h"
#include "inode.h"
#include "memory.h"
#include "print.h"
#include "string.h"

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

static void bitmap_format(size_t bid, size_t bcnt, size_t size, size_t occur) {
	void *buf = alloc_pages(bcnt);
	memset(buf, 1, bcnt * BLOCK_SIZE);
	memset(buf, 0, size);
	memset(buf, 1, occur);
	for (size_t i = 0; i != bcnt; ++i) {
		block_modify(0, bid + i, buf, BLOCK_SIZE, 0);
	}
	free_pages(buf, bcnt);
}

struct super_block *sb;
static void partition_format(void) {
	//	sb->magic = SB_MAGIC;
	size_t super_block_bid = cur_part->start_lba / SECTS_PER_BLOCK;
	size_t block_cnt = cur_part->sec_cnt / SECTS_PER_BLOCK;

	sb->block_bitmap_start = super_block_bid + 1;
	sb->block_bitmap_size = block_cnt;
	size_t block_bitmap_bcnt = DIV_ROUND_UP(block_cnt, BLOCK_SIZE);

	sb->inode_cnt_max = INODE_CNT_MAX;
	sb->inode_bitmap_start = sb->block_bitmap_start + block_bitmap_bcnt;
	size_t inode_bitmap_bcnt = DIV_ROUND_UP(INODE_CNT_MAX, BLOCK_SIZE);

	sb->inode_table_start = sb->inode_bitmap_start + inode_bitmap_bcnt;
	size_t inode_table_bcnt =
		DIV_ROUND_UP((INODE_CNT_MAX * DISK_INODE_SIZE), BLOCK_SIZE);

	sb->data_start = sb->inode_table_start + inode_table_bcnt;

	block_modify(0, super_block_bid, sb, BLOCK_SIZE, 0);
	bitmap_format(sb->block_bitmap_start, block_bitmap_bcnt, block_cnt,
		      1 + block_bitmap_bcnt + inode_bitmap_bcnt
			      + inode_table_bcnt);
	bitmap_format(sb->inode_bitmap_start, inode_bitmap_bcnt, INODE_CNT_MAX,
		      0);
}

static void create_root_dir(void) {
	ASSERT(disk_inode_create() == 0);

	struct inode *root_inode = inode_open(0);
	struct dirent de = { 0, "..", FT_DIR };
	ASSERT(dirent_add(root_inode, &de) > 0);
	de.filename[1] = 0;
	ASSERT(dirent_add(root_inode, &de) > 0);
	inode_close(root_inode);
}

static void pre_write_file(char *filename, size_t sec_start) {
	void *file_buf = alloc_pages(5);
	ASSERT(sys_mknod(filename, FT_REG, 0) >= 0);
	ide_read(0, sec_start, file_buf, 40);
	ssize_t fd = sys_open(filename);
	ASSERT(fd >= 0);
	sys_write(fd, file_buf, 5 * PG_SIZE);
	sys_close(fd);
	free_pages(file_buf, 5);
}

void filesys_init(void) {
	put_str("filesys_init: start\n");
	ASSERT(sizeof(struct disk_inode) <= DISK_INODE_SIZE);
	ASSERT(sizeof(struct dirent) <= DIRENT_SIZE);

	mbr_init();

	sb = alloc_pages(1);
	block_read(0, cur_part->start_lba / SECTS_PER_BLOCK, sb, BLOCK_SIZE, 0);
	bool hasfs = sb->magic == SB_MAGIC;
	if (!hasfs) {
		partition_format();
		block_read(0, cur_part->start_lba / SECTS_PER_BLOCK, sb,
			   BLOCK_SIZE, 0);
	}
	put_info("block_bitmap_start:\t0x", sb->block_bitmap_start);
	put_info("block_bitmap_size:\t0x", sb->block_bitmap_size);
	put_info("inode_bitmap_start:\t0x", sb->inode_bitmap_start);
	put_info("inode_table_start:\t0x", sb->inode_table_start);
	put_info("inode_cnt_max:\t\t0x", sb->inode_cnt_max);
	put_info("data_start:\t\t0x", sb->data_start);

	list_init(&open_inodes);
	sema_init(&open_inodes_lock, 1);
	sema_init(&fs_bitmap_lock, 1);

	file_table = alloc_pages(1);
	memset(file_table, 0, PG_SIZE);
	put_info("file table size:\t0x", FILE_TABLE_SIZE);
	sema_init(&file_table_lock, 1);

	if (!hasfs) {
		create_root_dir();
		ASSERT(sys_mkdir("/dev") == 0);
		ASSERT(sys_mknod("/dev/stdin", FT_CHR, 0) == 0);
		ASSERT(sys_mknod("/dev/stdout", FT_CHR, 1) == 1);

		pre_write_file("/init", 200);
		pre_write_file("/shell", 240);
	}
	put_str("filesys_init: end\n");
}
