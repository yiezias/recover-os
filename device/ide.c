#include "ide.h"
#include "debug.h"
#include "global.h"
#include "intr.h"
#include "io.h"
#include "memory.h"
#include "print.h"
#include "string.h"
#include "task.h"

#if 1
#define port_base(dev_no) (dev_no >= 2 ? 0x170 : 0x1f0)
#if 1 /* 定义硬盘各寄存器的端口号 */
#define reg_data(dev_no) (port_base(dev_no) + 0)
#define reg_error(dev_no) (port_base(dev_no) + 1)
#define reg_sect_cnt(dev_no) (port_base(dev_no) + 2)
#define reg_lba_l(dev_no) (port_base(dev_no) + 3)
#define reg_lba_m(dev_no) (port_base(dev_no) + 4)
#define reg_lba_h(dev_no) (port_base(dev_no) + 5)
#define reg_dev(dev_no) (port_base(dev_no) + 6)
#define reg_status(dev_no) (port_base(dev_no) + 7)
#define reg_cmd(dev_no) (reg_status(dev_no))
#define reg_alt_status(dev_no) (port_base(dev_no) + 0x206)
#define reg_ctl(dev_no) reg_alt_status(dev_no)
#endif

#if 1			    /* reg_status寄存器的一些关键位 */
#define BIT_STAT_BSY 0x80   // 硬盘忙
#define BIT_STAT_DRDY 0x40  // 驱动器准备好
#define BIT_STAT_DRQ 0x8    // 数据传输准备好了
#endif

#if 1			  /* device寄存器的一些关键位 */
#define BIT_DEV_MBS 0xa0  // 第7位和第5位固定为1
#define BIT_DEV_LBA 0x40
#define BIT_DEV_DEV 0x10
#endif

#if 1			       /* 一些硬盘操作的指令 */
#define CMD_IDENTIFY 0xec      // identify指令
#define CMD_READ_SECTOR 0x20   // 读扇区指令
#define CMD_WRITE_SECTOR 0x30  // 写扇区指令
#endif
#endif

static void select_disk(uint16_t dev_no) {
	uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
	if (dev_no % 2 == 1) {
		reg_device |= BIT_DEV_DEV;
	}
	outb(reg_dev(dev_no), reg_device);
}

static void select_sector(uint16_t dev_no, uint32_t lba, uint8_t sec_cnt) {
	outb(reg_sect_cnt(dev_no), sec_cnt);

	outb(reg_lba_l(dev_no), lba);
	outb(reg_lba_m(dev_no), lba >> 8);
	outb(reg_lba_h(dev_no), lba >> 16);
	outb(reg_dev(dev_no), BIT_DEV_MBS | BIT_DEV_LBA
				      | (dev_no % 2 ? BIT_DEV_DEV : 0)
				      | lba >> 24);
}

static bool busy_wait(uint16_t dev_no) {
	while (inb(reg_status(dev_no)) & BIT_STAT_BSY) {}
	return (inb(reg_status(dev_no)) & BIT_STAT_DRQ);
}

static void ide_read(uint16_t dev_no, void *buf, uint32_t lba,
		     uint32_t sec_cnt) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	select_disk(dev_no);
	select_sector(dev_no, lba, sec_cnt);
	outb(reg_cmd(dev_no), CMD_READ_SECTOR);
	ASSERT(busy_wait(dev_no));
	insw(reg_data(dev_no), buf, sec_cnt * 512 / 2);
	set_intr_stat(old_stat);
}

static void ide_write(uint16_t dev_no, void *buf, uint32_t lba,
		      uint32_t sec_cnt) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	select_disk(dev_no);
	select_sector(dev_no, lba, sec_cnt);
	outb(reg_cmd(dev_no), CMD_WRITE_SECTOR);
	ASSERT(busy_wait(dev_no));
	outsw(reg_data(dev_no), buf, sec_cnt * 512 / 2);
	set_intr_stat(old_stat);
}

#define default_dev_no 0
void disk_read(void *buf, uint32_t lba, uint32_t blk_cnt) {
	ide_read(default_dev_no, buf, lba * SECTS_PER_BLOCK,
		 blk_cnt * SECTS_PER_BLOCK);
}

void disk_write(void *buf, uint32_t lba, uint32_t blk_cnt) {
	ide_write(default_dev_no, buf, lba * SECTS_PER_BLOCK,
		  blk_cnt * SECTS_PER_BLOCK);
}


static void swap_pairs_bytes(const char *dst, char *buf, size_t len) {
	uint8_t idx;
	for (idx = 0; idx < len; idx += 2) {
		buf[idx + 1] = *dst++;
		buf[idx] = *dst++;
	}

	buf[idx] = '\0';
}

static void str_info(char *message, char *str) {
	put_str(message);
	put_str(str);
	put_char('\n');
}

uint32_t sectors;
static void identify_disk(uint16_t dev_no) {
	select_disk(dev_no);
	outb(reg_cmd(dev_no), CMD_IDENTIFY);
	ASSERT(busy_wait(dev_no));
	char id_info[512];
	insw(reg_data(dev_no), id_info, 256);

	char buf[64];
	uint8_t sn_start = 10 * 2, sn_len = 20, md_start = 27 * 2, md_len = 40;
	swap_pairs_bytes(&id_info[sn_start], buf, sn_len);

	put_info("\ndisk sda", dev_no);
	str_info("\tSN:\t\t", buf);
	memset(buf, 0, sizeof(buf));
	swap_pairs_bytes(&id_info[md_start], buf, md_len);
	str_info("\tMODULE:\t\t", buf);
	sectors = *(uint32_t *)&id_info[60 * 2];
	put_info("\tSECTORS:\t", sectors);
}

static void intr_hd_handle(void) {
	return;
}


void ide_init(void) {
	put_str("ide_init: start\n");
	uint8_t hd_cnt = *((uint8_t *)(0x475));

	register_intr_handle(0x2e, intr_hd_handle);

	for (uint8_t i = 0; i != hd_cnt; ++i) {
		identify_disk(i);
	}

	put_str("ide_init: done\n");
}
