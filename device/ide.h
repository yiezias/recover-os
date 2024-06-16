#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H
#include "types.h"

enum HD {
	ATA0M,
	ATA0S,
	ATA1M,
	ATA1S,
};

void ide_init(void);
void ide_read(enum HD hd, size_t lba, void *buf, size_t sec_cnt);
void ide_write(enum HD hd, size_t lba, void *buf, size_t sec_cnt);

#endif
