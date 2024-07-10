#ifndef __FS_BIO_H
#define __FS_BIO_H
#include "ide.h"
#include "types.h"

#define SEC_SIZE 512
#define BLOCK_SIZE 4096
#define SECTS_PER_BLOCK (BLOCK_SIZE / SEC_SIZE)

size_t block_read(enum HD hd, size_t bid, void *buf, size_t count, size_t off);
size_t block_modify(enum HD hd, size_t bid, const void *buf, size_t count,
		    size_t off);

#endif
