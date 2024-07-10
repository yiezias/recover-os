#ifndef __FS_BIO_H
#define __FS_BIO_H
#include "ide.h"
#include "types.h"

#define SEC_SIZE 512
#define BLOCK_SIZE PG_SIZE
#define SECTS_PER_BLOCK (BLOCK_SIZE / SEC_SIZE)

size_t block_read(enum HD hd, size_t bid, void *buf, size_t count, size_t pos);
size_t block_modify(enum HD hd, size_t bid, const void *buf, size_t count,
		    size_t pos);

#endif
