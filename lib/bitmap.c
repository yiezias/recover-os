#include "bitmap.h"
#include "debug.h"
#include "string.h"

void bitmap_init(struct bitmap *btmp) {
	memset(btmp->bits, 0, btmp->bytes_len);
}

bool bitmap_read(struct bitmap *btmp, uint64_t idx) {
	size_t bit_idx = idx % 8;
	size_t byte_idx = idx / 8;
	return (btmp->bits[byte_idx] >> bit_idx) & 1;
}

void bitmap_set(struct bitmap *btmp, uint64_t idx, bool value) {
	ASSERT(value == 0 || value == 1);
	size_t bit_idx = idx % 8;
	size_t byte_idx = idx / 8;
	if (value) {
		btmp->bits[byte_idx] |= (1 << bit_idx);
	} else {
		btmp->bits[byte_idx] &= ~(1 << bit_idx);
	}
}

int64_t bitmap_alloc(struct bitmap *btmp, uint64_t cnt) {
	int64_t ret = -1;
	for (size_t i = 0; i < 8 * btmp->bytes_len; ++i) {
		if (0 == bitmap_read(btmp, i)) {
			size_t j = 1;
			for (; j != cnt; ++j) {
				if (1 == bitmap_read(btmp, i + j)) {
					break;
				}
			}
			if (j == cnt) {
				for (size_t k = 0; k != cnt; ++k) {
					bitmap_set(btmp, i + k, 1);
				}
				ret = i;
				goto alloc_done;
			} else {
				i += j;
			}
		}
	}
alloc_done:
	return ret;
}
