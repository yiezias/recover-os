#include "dir.h"
#include "debug.h"
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

struct inode *path_prase(const char *pathname, ssize_t *i_no_ptr) {
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
		return NULL;
	}
	sema_down(&inode->inode_lock);
	ssize_t de_idx = dirent_search(inode, old_idx + 1);
	if (de_idx < 0) {
		*i_no_ptr = -FILE_ISNT_EXIST;
	} else {
		struct dirent *de = kalloc(DIRENT_SIZE);
		inode_read(inode, de, sizeof(struct dirent),
			   de_idx * DIRENT_SIZE);
		*i_no_ptr = de->i_no;
		kfree(de);
	}
	sema_up(&inode->inode_lock);
	return inode;
}
