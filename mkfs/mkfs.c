#if 1 /* 系统头文件 */
#include </usr/include/string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#endif

#define __LIB_TYPES_H
#define __LIB_STRING_H
#include "ide.h"
#include "super_block.h"

#define DIV_ROUND_UP(X, STEP) (((X) + (STEP)-1) / (STEP))

uint8_t *disk;
uint32_t disk_size;
uint32_t blk_cnt;

#if 1 /* 硬盘初始化 */

static void disk_init(char *filename) {
	struct stat filestat;
	stat(filename, &filestat);
	disk_size = filestat.st_size;
	assert(disk_size == 10321920);
	blk_cnt = disk_size / BLOCK_SIZE;

	printf("disk size: %d\n", disk_size);

	disk = malloc(disk_size);

	FILE *fdisk = fopen(filename, "rb");
	assert(fdisk);
	size_t rd_cnt = fread(disk, BLOCK_SIZE, blk_cnt, fdisk);
	assert(rd_cnt == blk_cnt);
	fclose(fdisk);
}

static void disk_sync(char *filename) {
	FILE *fdisk = fopen(filename, "wb");
	assert(fdisk);
	size_t wr_cnt = fwrite(disk, BLOCK_SIZE, blk_cnt, fdisk);
	assert(wr_cnt == blk_cnt);
	fclose(fdisk);
	free(disk);
}

#endif

#if 1 /* 硬盘读写 */
void disk_read(void *buf, uint32_t lba, uint32_t blk_cnt) {
	memcpy(buf, disk + lba * BLOCK_SIZE, blk_cnt * BLOCK_SIZE);
}

void disk_write(void *buf, uint32_t lba, uint32_t blk_cnt) {
	memcpy(disk + lba * BLOCK_SIZE, buf, blk_cnt * BLOCK_SIZE);
}
#endif


static void bitmap_set(uint8_t *bits, size_t idx, uint8_t value) {
	assert(value == 0 || value == 1);
	size_t bit_idx = idx % 8;
	size_t byte_idx = idx / 8;
	if (value) {
		bits[byte_idx] |= (1 << bit_idx);
	} else {
		bits[byte_idx] &= ~(1 << bit_idx);
	}
}

static uint8_t bitmap_read(uint8_t *bits, size_t idx) {
	size_t bit_idx = idx % 8;
	size_t byte_idx = idx / 8;
	return (bits[byte_idx] >> bit_idx) & 1;
}

static size_t bitmap_alloc(uint8_t *bits) {
	for (size_t i = 0;; ++i) {
		if (bitmap_read(bits, i) == 0) {
			return i;
		}
	}
}

struct super_block sb;

static uint32_t balloc(void) {
	return bitmap_alloc(disk + sb.block_bitmap_lba);
}

static void check_and_format(void) {
	void *sb_buf = malloc(BLOCK_SIZE);
	disk_read(sb_buf, SB_LBA, 1);
	memcpy(&sb, sb_buf, sizeof(sb));
	free(sb_buf);

	if (sb.magic == SB_MAGIC) {
		// return;
	}

	sb.magic = SB_MAGIC;
	sb.block_bitmap_blks = DIV_ROUND_UP(blk_cnt, BLOCK_SIZE);
	assert(sb.block_bitmap_blks == 1);
	printf("blk_cnt: %u\n", blk_cnt);
	sb.block_bitmap_lba = SB_LBA + 1;
	printf("block_bitmap_lba %u, block_bitmap_blks %u\n",
	       sb.block_bitmap_lba, sb.block_bitmap_blks);

	uint8_t *block_bitmap = malloc(sb.block_bitmap_blks * BLOCK_SIZE);
	memset(block_bitmap, 0, sb.block_bitmap_blks);
	for (size_t i = blk_cnt; i != sb.block_bitmap_blks * BLOCK_SIZE; ++i) {
		if (i >= blk_cnt
		    || i < sb.block_bitmap_lba + sb.block_bitmap_blks) {
			bitmap_set(block_bitmap, i, 1);
		}
	}
	disk_write(block_bitmap, sb.block_bitmap_lba, sb.block_bitmap_blks);
}

int main(int argc, char *argv[]) {
	assert(argc >= 2);

	char *filename = argv[1];

	disk_init(filename);

	check_and_format();

	disk_sync(filename);
	return 0;
}
