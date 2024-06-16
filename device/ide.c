#include "ide.h"
#include "debug.h"
#include "intr.h"
#include "io.h"
#include "print.h"
#include "sync.h"
#include "timer.h"

#define port_base(hd) (0x1f0 + 0x80 * (hd / 2))
/* 定义硬盘各寄存器的端口号 */
#define reg_data(hd) (port_base(hd) + 0)
#define reg_error(hd) (port_base(hd) + 1)
#define reg_sect_cnt(hd) (port_base(hd) + 2)
#define reg_lba_l(hd) (port_base(hd) + 3)
#define reg_lba_m(hd) (port_base(hd) + 4)
#define reg_lba_h(hd) (port_base(hd) + 5)
#define reg_dev(hd) (port_base(hd) + 6)
#define reg_status(hd) (port_base(hd) + 7)
#define reg_cmd(hd) (reg_status(hd))

/* reg_status寄存器的一些关键位 */
#define BIT_STAT_BSY 0x80   // 硬盘忙
#define BIT_STAT_DRDY 0x40  // 驱动器准备好
#define BIT_STAT_DRQ 0x8    // 数据传输准备好了

/* device寄存器的一些关键位 */
#define BIT_DEV_MBS 0xa0  // 第7位和第5位固定为1
#define BIT_DEV_LBA 0x40
#define BIT_DEV_DEV 0x10

/* 一些硬盘操作的指令 */
#define CMD_IDENTIFY 0xec      // identify指令
#define CMD_READ_SECTOR 0x20   // 读扇区指令
#define CMD_WRITE_SECTOR 0x30  // 写扇区指令

uint8_t hd_cnt;
size_t disk_sectors;

struct ide_channel {
	struct semaphore lock;
	bool expecting_intr;
	struct semaphore disk_done;
};
struct ide_channel channels[2];


static void select_disk(enum HD hd) {
	uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
	if (hd == 1) {
		reg_device |= BIT_DEV_DEV;
	}
	outb(reg_dev(hd), reg_device);
}

static void select_sector(enum HD hd, size_t lba, uint8_t sec_cnt) {
	outb(reg_sect_cnt(hd), sec_cnt);

	outb(reg_lba_l(hd), lba);
	outb(reg_lba_m(hd), lba >> 8);
	outb(reg_lba_h(hd), lba >> 16);
	outb(reg_dev(hd), BIT_DEV_MBS | BIT_DEV_LBA
				  | (hd == 1 ? BIT_DEV_DEV : 0) | lba >> 24);
}

static bool busy_wait(enum HD hd) {
	uint16_t time_limit = 30 * 1000;

	while (time_limit -= 10 > 0) {
		if (!(inb(reg_status(hd)) & BIT_STAT_BSY)) {
			return (inb(reg_status(hd)) & BIT_STAT_DRQ);
		} else {
			mtime_sleep(10);
		}
	}
	return false;
}

static void cmd_out(enum HD hd, uint8_t cmd) {
	channels[hd / 2].expecting_intr = true;
	outb(reg_cmd(hd), cmd);
}

static void read_from_sector(enum HD hd, void *buf, uint8_t sec_cnt) {
	uint32_t size_in_byte;
	if (sec_cnt == 0) {
		size_in_byte = 256 * 512;
	} else {
		size_in_byte = sec_cnt * 512;
	}
	insw(reg_data(hd), buf, size_in_byte / 2);
}

static void write2sector(enum HD hd, void *buf, uint8_t sec_cnt) {
	uint32_t size_in_byte;
	if (sec_cnt == 0) {
		size_in_byte = 256 * 512;
	} else {
		size_in_byte = sec_cnt * 512;
	}
	outsw(reg_data(hd), buf, size_in_byte / 2);
}


void ide_read(enum HD hd, size_t lba, void *buf, size_t sec_cnt) {
	ASSERT(sec_cnt > 0 && hd < hd_cnt);
	sema_down(&channels[hd / 2].lock);

	select_disk(hd);
	for (size_t secs_done = 0; secs_done <= sec_cnt; secs_done += 256) {
		int secs_op = (sec_cnt - secs_done) >= 256
				      ? 256
				      : (sec_cnt - secs_done);
		select_sector(hd, lba + secs_done, secs_op);
		cmd_out(hd, CMD_READ_SECTOR);
		sema_down(&channels[hd / 2].disk_done);
		ASSERT(busy_wait(hd));
		read_from_sector(hd, buf + secs_done * 512, secs_op);
	}

	sema_up(&channels[hd / 2].lock);
}

void ide_write(enum HD hd, size_t lba, void *buf, size_t sec_cnt) {
	ASSERT(sec_cnt > 0 && hd < hd_cnt);
	sema_down(&channels[hd / 2].lock);

	select_disk(hd);
	for (size_t secs_done = 0; secs_done <= sec_cnt; secs_done += 256) {
		int secs_op = (sec_cnt - secs_done) >= 256
				      ? 256
				      : (sec_cnt - secs_done);
		select_sector(hd, lba + secs_done, secs_op);
		cmd_out(hd, CMD_WRITE_SECTOR);
		ASSERT(busy_wait(hd));
		write2sector(hd, buf + secs_done * 512, secs_op);
		sema_down(&channels[hd / 2].disk_done);
	}

	sema_up(&channels[hd / 2].lock);
}

static size_t identify_disk(enum HD hd) {
	char id_info[512];
	select_disk(hd);
	cmd_out(hd, CMD_IDENTIFY);
	sema_down(&channels[hd / 2].disk_done);
	ASSERT(busy_wait(hd));
	read_from_sector(hd, id_info, 1);

	put_str("disk sda");
	put_char('0' + hd);
	put_str(" sectors: 0x");
	size_t sectors = *(uint32_t *)&id_info[60 * 2];
	put_num(sectors);
	put_str("\n");
	return sectors;
}

static void intr_hd_handle(uint8_t irq_no) {
	ASSERT(irq_no == 0x2e || irq_no == 0x2f);
	uint8_t ch_no = irq_no - 0x2e;
	struct ide_channel *channel = &channels[ch_no];
	if (channel->expecting_intr) {
		channel->expecting_intr = false;
		sema_up(&channel->disk_done);
		inb(reg_status(ch_no * 2));
	}
}


void ide_init(void) {
	put_str("ide_init: start\n");

	hd_cnt = *((uint8_t *)(0x475));
	for (int i = 0; i != 2; ++i) {
		sema_init(&channels[i].lock, 1);
		channels[i].expecting_intr = false;
		sema_init(&channels[i].disk_done, 0);
		register_intr_handle(0x2e + i, intr_hd_handle);
	}
	for (int i = 0; i != hd_cnt; ++i) {
		size_t sectors = identify_disk(i);
		if (i == 0) {
			disk_sectors = sectors;
		}
	}

	put_str("ide_init: end\n");
}
