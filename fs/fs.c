#include "fs.h"
#include "debug.h"
#include "ide.h"
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

void filesys_init(void) {
	put_str("filesys_init: start\n");

	mbr_init();

	put_str("filesys_init: end\n");
}
