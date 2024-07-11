#include "inode.h"
#include "bio.h"
#include "debug.h"
#include "fs.h"
#include "global.h"
#include "list.h"
#include "memory.h"
#include "string.h"

#define block_id(i_no) \
	((i_no * DISK_INODE_SIZE) / BLOCK_SIZE + sb->inode_table_start)
#define block_off(i_no) ((i_no * DISK_INODE_SIZE) % BLOCK_SIZE)

struct list open_inodes;
struct semaphore open_inodes_lock;


static bool find_inode(struct list_elem *elem, void *p_no) {
	return (elem2entry(struct inode, inode_tag, elem))->i_no
	       == *(size_t *)p_no;
}

struct inode *inode_open(size_t i_no) {
	if (i_no >= INODE_CNT_MAX) {
		return NULL;
	}

	sema_down(&open_inodes_lock);
	struct list_elem *inode_tag =
		list_traversal(&open_inodes, find_inode, &i_no);
	struct inode *inode = NULL;
	if (inode_tag == NULL) {
		inode = kalloc(sizeof(struct inode));
		block_read(0, block_id(i_no), &inode->disk_inode,
			   sizeof(struct disk_inode), block_off(i_no));

		inode->i_no = i_no;
		inode->open_cnts = 0;
		sema_init(&inode->inode_lock, 1);
		list_push(&open_inodes, &inode->inode_tag);

	} else {
		inode = elem2entry(struct inode, inode_tag, inode_tag);
	}
	sema_up(&open_inodes_lock);

	sema_down(&inode->inode_lock);
	++inode->open_cnts;
	sema_up(&inode->inode_lock);

	return inode;
}

void inode_close(struct inode *inode) {
	sema_down(&inode->inode_lock);
	--inode->open_cnts;
	if (inode->open_cnts == 0) {
		sema_down(&open_inodes_lock);
		list_remove(&inode->inode_tag);
		sema_up(&open_inodes_lock);
		kfree(inode);
	}
	sema_up(&inode->inode_lock);
}

struct semaphore fs_bitmap_lock;
static ssize_t fs_bitmap_alloc(size_t start, size_t size) {
	size_t i = 0;
	uint8_t buf = 1;
	sema_down(&fs_bitmap_lock);
	for (; i != size; ++i) {
		block_read(0, i / BLOCK_SIZE + start, &buf, 1, i % BLOCK_SIZE);
		if (buf == 0) {
			buf = 1;
			break;
		}
	}
	if (i == size) {
		sema_up(&fs_bitmap_lock);
		return -NO_BIT_IN_FS_BTMP;
	}
	block_modify(0, i / BLOCK_SIZE + start, &buf, 1, i % BLOCK_SIZE);
	sema_up(&fs_bitmap_lock);
	return i;
}
static void fs_bitmap_free(size_t start, size_t size, size_t idx) {
	ASSERT(idx < size);
	uint8_t buf = 0;
	sema_down(&fs_bitmap_lock);
	block_modify(0, idx / BLOCK_SIZE + start, &buf, 1, idx % BLOCK_SIZE);
	sema_up(&fs_bitmap_lock);
}

ssize_t inode_size_change(struct inode *inode, size_t size) {
	if (size > NDIRECT * BLOCK_SIZE) {
		return -FILE_SIZE_TOO_BIG;
	}
	size_t block_old_size =
		DIV_ROUND_UP(inode->disk_inode.file_size, PG_SIZE);
	size_t block_new_size = DIV_ROUND_UP(size, PG_SIZE);
	if (block_new_size > block_old_size) {
		for (size_t idx = block_old_size; idx != block_new_size;
		     ++idx) {
			ssize_t bidx = fs_bitmap_alloc(sb->block_bitmap_start,
						       sb->block_bitmap_size);
			if (bidx < 0) {
				return bidx;
			}
			inode->disk_inode.direct[idx] =
				bidx + sb->block_bitmap_start - 1;
		}
	} else if (block_new_size < block_old_size) {
		for (size_t idx = block_new_size; idx != block_old_size;
		     ++idx) {
			fs_bitmap_free(sb->block_bitmap_start,
				       sb->block_bitmap_size,
				       inode->disk_inode.direct[idx]
					       - (sb->block_bitmap_start - 1));
		}
	}

	inode->disk_inode.file_size = size;
	block_modify(0, block_id(inode->i_no), &inode->disk_inode,
		     sizeof(struct disk_inode), block_off(inode->i_no));
	return 0;
}

ssize_t inode_write(struct inode *inode, const void *buf, size_t nbyte,
		    size_t offset) {
	if (offset + nbyte > inode->disk_inode.file_size) {
		ssize_t success = inode_size_change(inode, offset + nbyte);
		if (success < 0) {
			return success;
		}
	}
	size_t block_start = offset / BLOCK_SIZE;
	size_t block_cnt =
		DIV_ROUND_UP(offset + nbyte, BLOCK_SIZE) - block_start;
	void *io_buf = alloc_pages(block_cnt);
	for (size_t i = 0; i != block_cnt; ++i) {
		block_read(0, inode->disk_inode.direct[block_start + i],
			   io_buf + i * PG_SIZE, BLOCK_SIZE, 0);
	}
	memcpy(io_buf + offset % BLOCK_SIZE, buf, nbyte);
	for (size_t i = 0; i != block_cnt; ++i) {
		block_modify(0, inode->disk_inode.direct[block_start + i],
			     io_buf + i * PG_SIZE, BLOCK_SIZE, 0);
	}
	free_pages(io_buf, block_cnt);
	return nbyte;
}

ssize_t inode_read(struct inode *inode, void *buf, size_t nbyte,
		   size_t offset) {
	if (offset >= inode->disk_inode.file_size) {
		return 0;
	} else if (nbyte + offset > inode->disk_inode.file_size) {
		nbyte = inode->disk_inode.file_size - offset;
	}

	size_t block_start = offset / BLOCK_SIZE;
	size_t block_cnt =
		DIV_ROUND_UP(offset + nbyte, BLOCK_SIZE) - block_start;
	void *io_buf = alloc_pages(block_cnt);

	for (size_t i = 0; i != block_cnt; ++i) {
		block_read(0, inode->disk_inode.direct[block_start + i],
			   io_buf + i * PG_SIZE, BLOCK_SIZE, 0);
	}
	memcpy(buf, io_buf + offset % BLOCK_SIZE, nbyte);
	free_pages(io_buf, block_cnt);
	return nbyte;
}

ssize_t disk_inode_create(void) {
	ssize_t idx = fs_bitmap_alloc(sb->inode_bitmap_start, INODE_CNT_MAX);
	if (idx < 0) {
		return idx;
	}
	struct disk_inode inode = { 0 };
	block_modify(0, block_id(idx), &inode, sizeof(struct disk_inode),
		     block_off(idx));
	return idx;
}

void disk_inode_delete(size_t i_no) {
	fs_bitmap_free(sb->inode_bitmap_start, INODE_CNT_MAX, i_no);
	struct inode *inode = inode_open(i_no);
	sema_down(&inode->inode_lock);
	inode_size_change(inode, 0);
	sema_up(&inode->inode_lock);
	inode_close(inode);
}
