#ifndef __LIB_BITMAP_H
#define __LIB_BITMAP_H
#include "types.h"

struct bitmap {
	size_t bytes_len;
	uint8_t *bits;
};

/* 初始化位图结构 */
void bitmap_init(struct bitmap *btmp);
/* 获取索引位 */
bool bitmap_read(struct bitmap *btmp, uint64_t idx);
/* 设置索引位 */
void bitmap_set(struct bitmap *btmp, uint64_t idx, bool value);
/* 获取连续0索引位并置1 */
int64_t bitmap_alloc(struct bitmap *btmp, uint64_t cnt);

#endif
