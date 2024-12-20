#include "bio.h"
#include "memory.h"
#include "string.h"

size_t block_read(enum HD hd, size_t bid, void *buf, size_t count, size_t off) {
	if (off > BLOCK_SIZE) {
		return 0;
	} else if (off + count > BLOCK_SIZE) {
		count = BLOCK_SIZE - off;
	}
	void *iobuf = alloc_pages(1);
	ide_read(hd, bid * SECTS_PER_BLOCK, iobuf, SECTS_PER_BLOCK);

	memcpy(buf, iobuf + off, count);

	free_pages(iobuf, 1);
	return count;
}

size_t block_modify(enum HD hd, size_t bid, const void *buf, size_t count,
		    size_t off) {
	if (off > BLOCK_SIZE) {
		return 0;
	} else if (off + count > BLOCK_SIZE) {
		count = BLOCK_SIZE - off;
	}
	void *iobuf = alloc_pages(1);
	ide_read(hd, bid * SECTS_PER_BLOCK, iobuf, SECTS_PER_BLOCK);

	memcpy(iobuf + off, buf, count);

	ide_write(hd, bid * SECTS_PER_BLOCK, iobuf, SECTS_PER_BLOCK);
	free_pages(iobuf, 1);
	return count;
}
