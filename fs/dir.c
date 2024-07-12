#include "dir.h"
#include "debug.h"
#include "global.h"
#include "memory.h"
#include "string.h"

ssize_t dirent_search(struct inode *dir_inode, const char *name) {
	ssize_t idx = 0;
	ssize_t dir_size = dir_inode->disk_inode.file_size;
	for (; idx != dir_size / DIRENT_SIZE; ++idx) {
		uint8_t de[DIRENT_SIZE];
		ssize_t nb = inode_read(dir_inode, de, sizeof(struct dirent),
					idx * DIRENT_SIZE);
		ASSERT(nb == sizeof(struct dirent));
		struct dirent *e = (struct dirent *)de;
		if (!strcmp(e->filename, name)) {
			break;
		}
	}
	if (idx == dir_size / DIRENT_SIZE) {
		return -DIRENT_ISNT_EXIST;
	}
	return idx;
}

ssize_t dirent_add(struct inode *dir_inode, struct dirent *de) {
	if (dirent_search(dir_inode, de->filename) == -DIRENT_ISNT_EXIST) {
		return inode_write(dir_inode, de, DIRENT_SIZE,
				   dir_inode->disk_inode.file_size);
	} else {
		return -DIRENT_ALREADY_EXIST;
	}
}

ssize_t dirent_delete(struct inode *dir_inode, const char *name) {
	size_t dir_size = dir_inode->disk_inode.file_size;
	ssize_t idx = dirent_search(dir_inode, name);
	if (idx == -DIRENT_ISNT_EXIST) {
		return idx;
	}
	struct dirent end_de;
	inode_read(dir_inode, &end_de, sizeof(struct dirent),
		   dir_size - DIRENT_SIZE);
	inode_write(dir_inode, &end_de, sizeof(struct dirent),
		    idx * DIRENT_SIZE);
	inode_size_change(dir_inode, dir_size - DIRENT_SIZE);
	return 0;
}

ssize_t path_prase(const char *pathname, struct dirent *de_buf) {
	char *old_idx = (char *)pathname;
	char *idx = old_idx + 1;
	struct inode *inode = inode_open(0);
	for (; *idx != 0; ++idx) {
		if (*idx == '/') {
			size_t name_len = idx - old_idx;
			char *name = kalloc(name_len);
			memcpy(name, old_idx + 1, name_len);
			name[name_len - 1] = 0;

			sema_down(&inode->inode_lock);
			ssize_t dir_idx = dirent_search(inode, name);
			if (dir_idx < 0) {
				break;
			}
			struct dirent *de = kalloc(DIRENT_SIZE);
			inode_read(inode, de, sizeof(struct dirent),
				   dir_idx * DIRENT_SIZE);
			sema_up(&inode->inode_lock);
			if (de->f_type != FT_DIR) {
				break;
			}
			inode_close(inode);
			inode = inode_open(de->i_no);
			kfree(de);

			kfree(name);
			old_idx = idx;
		}
	}
	if (*idx != 0) {
		inode_close(inode);
		return -DIRENT_ISNT_EXIST;
	}
	sema_down(&inode->inode_lock);
	ssize_t de_idx = dirent_search(inode, old_idx + 1);
	if (de_idx < 0) {
		de_buf->i_no = ULONG_MAX;
		memcpy(de_buf->filename, old_idx + 1, idx - old_idx);
	} else {
		inode_read(inode, de_buf, sizeof(struct dirent),
			   de_idx * DIRENT_SIZE);
	}
	size_t p_i_no = inode->i_no;
	sema_up(&inode->inode_lock);
	return p_i_no;
}

ssize_t sys_mkdir(char *pathname) {
	/* path必须目标不存在但父目录存在 */
	struct dirent de_buf;
	ssize_t p_i_no = path_prase(pathname, &de_buf);
	if (p_i_no == -DIRENT_ISNT_EXIST) {
		return p_i_no;
	} else if (de_buf.i_no != ULONG_MAX) {
		return -DIRENT_ALREADY_EXIST;
	}
	/* 分配inode */
	struct inode *parent_inode = inode_open(p_i_no);
	sema_down(&parent_inode->inode_lock);
	ssize_t i_no = disk_inode_create();
	if (i_no < 0) {
		sema_up(&parent_inode->inode_lock);
		inode_close(parent_inode);
		return i_no;
	}
	/* 父目录添加目录项 */
	struct dirent de = { i_no, { 0 }, FT_DIR };
	strcpy(de.filename, de_buf.filename);
	dirent_add(parent_inode, &de);

	sema_up(&parent_inode->inode_lock);
	inode_close(parent_inode);

	/* 子目录添加.和..两个目录项 */
	struct dirent dot = { i_no, ".", FT_DIR };
	struct dirent ddot = { p_i_no, "..", FT_DIR };
	struct inode *inode = inode_open(i_no);
	sema_down(&inode->inode_lock);
	dirent_add(inode, &dot);
	dirent_add(inode, &ddot);
	sema_up(&inode->inode_lock);
	inode_close(inode);

	return 0;
}

ssize_t sys_rmdir(char *pathname) {
	struct dirent de_buf;
	ssize_t p_i_no = path_prase(pathname, &de_buf);
	if (p_i_no == -DIRENT_ISNT_EXIST) {
		return p_i_no;
	} else if (de_buf.i_no == ULONG_MAX) {
		return -FILE_ISNT_EXIST;
	}

	/* 判断搜索到的是不是目录 */
	if (de_buf.f_type != FT_DIR) {
		return -ISNT_DIR;
	}
	/* 判断目录是否为空 */
	struct inode *inode = inode_open(de_buf.i_no);
	sema_down(&inode->inode_lock);
	bool empty = (inode->disk_inode.file_size == 2 * DIRENT_SIZE);
	sema_up(&inode->inode_lock);
	inode_close(inode);
	if (!empty) {
		return -DIR_ISNT_EMPTY;
	}
	/* 准备就绪 */
	disk_inode_delete(de_buf.i_no);
	struct inode *parent_inode = inode_open(p_i_no);
	sema_down(&parent_inode->inode_lock);

	dirent_delete(parent_inode, de_buf.filename);

	sema_up(&parent_inode->inode_lock);
	inode_close(parent_inode);
	return 0;
}

ssize_t sys_stat(char *pathname, struct stat *stat_buf) {
	struct dirent de_buf;
	ssize_t p_i_no = path_prase(pathname, &de_buf);
	if (p_i_no == -DIRENT_ISNT_EXIST) {
		return p_i_no;
	} else if (de_buf.i_no == ULONG_MAX) {
		return -FILE_ISNT_EXIST;
	}

	stat_buf->ino = de_buf.i_no;
	stat_buf->file_type = de_buf.f_type;
	struct inode *inode = inode_open(de_buf.i_no);
	sema_down(&inode->inode_lock);
	stat_buf->size = inode->disk_inode.file_size;
	sema_up(&inode->inode_lock);
	inode_close(inode);
	return 0;
}
