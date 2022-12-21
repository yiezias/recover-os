#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H
#include "types.h"

void ide_init(void);

#define SECTOR_SIZE 512
#define BLOCK_SIZE 4096
#define SECTS_PER_BLOCK (BLOCK_SIZE / SECTOR_SIZE)

void disk_read(void *buf, uint32_t lba, uint32_t blk_cnt);
void disk_write(void *buf, uint32_t lba, uint32_t blk_cnt);

extern uint32_t sectors;

#endif
