#include "file.h"
#include "console.h"
#include "debug.h"
#include "dir.h"
#include "global.h"
#include "pipe.h"
#include "string.h"
#include "task.h"

struct file *file_table;
struct semaphore file_table_lock;

ssize_t get_free_slot(struct inode *inode, enum file_types f_type) {
	ssize_t i = 0;
	for (; i != FILE_TABLE_SIZE; ++i) {
		if (file_table[i].f_inode == NULL) {
			break;
		}
	}
	if (i != FILE_TABLE_SIZE) {
		file_table[i].f_inode = inode;
		file_table[i].f_pos = 0;
		file_table[i].f_type = f_type;
	}
	return i;
}

ssize_t free_fd_alloc(void) {
	struct task_struct *cur_task = running_task();
	ssize_t i = 0;
	for (; i != MAX_FILES_OPEN_PER_PROC; ++i) {
		if (cur_task->fd_table[i] == -1) {
			break;
		}
	}
	return i;
}

ssize_t sys_mknod(const char *pathname, enum file_types type, ssize_t dev) {
	if (type != FT_CHR && type != FT_REG) {
		return -FILE_TYPE_INVALID;
	}
	/* path必须目标不存在但父目录存在 */
	struct dirent de_buf;
	ssize_t p_i_no = path_prase(pathname, &de_buf);
	if (p_i_no == -DIRENT_ISNT_EXIST) {
		return p_i_no;
	} else if (de_buf.i_no != ULONG_MAX) {
		return -DIRENT_ALREADY_EXIST;
	}
	/* 分配inode */
	if (type == FT_REG) {
		dev = disk_inode_create();
		if (dev < 0) {
			return dev;
		}
	}
	/* 父目录添加目录项 */
	struct inode *parent_inode = inode_open(p_i_no);
	sema_down(&parent_inode->inode_lock);
	struct dirent de = { dev, { 0 }, type };
	strcpy(de.filename, de_buf.filename);
	ssize_t add_suc = dirent_add(parent_inode, &de);
	if (add_suc != DIRENT_SIZE) {
		if (type == FT_REG) {
			disk_inode_delete(dev);
		}
		sema_up(&parent_inode->inode_lock);
		inode_close(parent_inode);
		return add_suc;
	}
	sema_up(&parent_inode->inode_lock);
	inode_close(parent_inode);

	return dev;
}

ssize_t sys_unlink(const char *pathname) {
	struct dirent de_buf;
	ssize_t p_i_no = path_prase(pathname, &de_buf);
	if (p_i_no == -DIRENT_ISNT_EXIST) {
		return p_i_no;
	} else if (de_buf.i_no == ULONG_MAX) {
		return -FILE_ISNT_EXIST;
	}

	/* 判断搜索到的是不是目录 */
	if (de_buf.f_type == FT_DIR) {
		return -ISNT_DIR;
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

ssize_t sys_open(const char *pathname) {
	/* 搜索一个存在的文件 */
	struct dirent de_buf;
	ssize_t p_i_no = path_prase(pathname, &de_buf);
	if (p_i_no < 0 || de_buf.i_no == ULONG_MAX) {
		return -FILE_ISNT_EXIST;
	}
	/* 找一个空闲文件描述符 */
	ssize_t fd = free_fd_alloc();
	if (fd == MAX_FILES_OPEN_PER_PROC) {
		return -fd;
	}
	/* 从文件表中找空闲位 */
	struct inode *inode = de_buf.f_type == FT_CHR ? (struct inode *)&console
						      : inode_open(de_buf.i_no);
	sema_down(&file_table_lock);
	ssize_t ft_idx = get_free_slot(inode, de_buf.f_type);
	if (de_buf.f_type == FT_CHR) {
		file_table[ft_idx].f_pos = 1;
	}
	sema_up(&file_table_lock);
	if (ft_idx == FILE_TABLE_SIZE) {
		return -ft_idx;
	}
	/* 将空闲位装载到进程中 */
	running_task()->fd_table[fd] = ft_idx;
	return fd;
}

ssize_t sys_close(ssize_t fd) {
	struct task_struct *cur_task = running_task();
	ssize_t ft_idx = cur_task->fd_table[fd];
	/* 判断参数是否合法 */
	if (fd < 0 || fd > MAX_FILES_OPEN_PER_PROC || ft_idx < 0
	    || ft_idx >= (ssize_t)FILE_TABLE_SIZE) {
		return -FD_INVALID;
	}
	/* 置文件表对应位为空闲 */
	sema_down(&file_table_lock);
	ASSERT(file_table[ft_idx].f_inode != NULL);

	bool clear_f_inode = false;
	if (file_table[ft_idx].f_type == FT_CHR) {
		if (--file_table[ft_idx].f_pos == 0) {
			clear_f_inode = true;
		}
	} else if (file_table[ft_idx].f_type == FT_FIFO) {
		if (--file_table[ft_idx].f_pos == 0) {
			free_pages(file_table[ft_idx].f_inode, 1);
			clear_f_inode = true;
		}
	} else {
		inode_close(file_table[ft_idx].f_inode);
		clear_f_inode = true;
	}
	if (clear_f_inode) {
		file_table[ft_idx].f_inode = NULL;
	}
	sema_up(&file_table_lock);
	/* 置进程文件描述符为空闲 */
	cur_task->fd_table[fd] = -1;
	return 0;
}

ssize_t sys_lseek(ssize_t fd, ssize_t offset, enum whence whence) {
	ssize_t new_pos = 0;
	ssize_t ft_idx = running_task()->fd_table[fd];
	/* 判断参数是否合法 */
	if (fd < 0 || fd > MAX_FILES_OPEN_PER_PROC || ft_idx < 0
	    || ft_idx >= (ssize_t)FILE_TABLE_SIZE) {
		return -FD_INVALID;
	}
	sema_down(&file_table_lock);
	if (file_table[ft_idx].f_type != FT_REG
	    && file_table[ft_idx].f_type != FT_DIR) {
		sema_up(&file_table_lock);
		return 0;
	}
	struct file *pf = file_table + ft_idx;
	ssize_t file_size = pf->f_inode->disk_inode.file_size;
	switch (whence) {
	case SEEK_SET:
		new_pos = offset;
		break;
	case SEEK_CUR:
		new_pos = pf->f_pos + offset;
		break;
	case SEEK_END:
		new_pos = file_size + offset;
		break;
	}
	if (new_pos < 0 || new_pos > file_size) {
		sema_up(&file_table_lock);
		return -1;
	}
	pf->f_pos = new_pos;
	sema_up(&file_table_lock);
	return new_pos;
}

ssize_t sys_read(ssize_t fd, void *buf, size_t count) {
	struct task_struct *cur_task = running_task();
	ssize_t ft_idx = cur_task->fd_table[fd];
	/* 判断参数是否合法 */
	if (fd < 0 || fd > MAX_FILES_OPEN_PER_PROC || ft_idx < 0
	    || ft_idx >= (ssize_t)FILE_TABLE_SIZE) {
		return -FD_INVALID;
	}
	sema_down(&file_table_lock);
	struct file *pf = file_table + ft_idx;
	struct inode *inode = pf->f_inode;
	ssize_t nbyte = 0;
	/* 进行读操作 */
	if (pf->f_type == FT_DIR || pf->f_type == FT_REG) {
		sema_down(&inode->inode_lock);
		nbyte = inode_read(inode, buf, count, pf->f_pos);
		sema_up(&inode->inode_lock);
		if (nbyte > 0) {
			pf->f_pos += nbyte;
		}
	}
	enum file_types f_type = pf->f_type;
	sema_up(&file_table_lock);

	if (f_type == FT_CHR) {
		nbyte = count;
		console_read(buf, count);
	} else if (f_type == FT_FIFO) {
		nbyte = pipe_read(fd, buf, count);
	}

	return nbyte;
}

ssize_t sys_write(ssize_t fd, void *buf, size_t count) {
	struct task_struct *cur_task = running_task();
	ssize_t ft_idx = cur_task->fd_table[fd];
	/* 判断参数是否合法 */
	if (fd < 0 || fd > MAX_FILES_OPEN_PER_PROC || ft_idx < 0
	    || ft_idx >= (ssize_t)FILE_TABLE_SIZE) {
		return -FD_INVALID;
	}
	sema_down(&file_table_lock);
	struct file *pf = file_table + ft_idx;
	struct inode *inode = pf->f_inode;
	ssize_t nbyte = 0;
	/* 进行写操作 */
	if (pf->f_type == FT_REG) {
		sema_down(&inode->inode_lock);
		nbyte = inode_write(inode, buf, count, pf->f_pos);
		sema_up(&inode->inode_lock);
		if (nbyte > 0) {
			pf->f_pos += nbyte;
		}
	} else if (pf->f_type == FT_CHR) {
		nbyte = count;
		console_write(buf, count);
	} else if (pf->f_type == FT_FIFO) {
		nbyte = pipe_write(fd, buf, count);
	}

	sema_up(&file_table_lock);
	return nbyte;
}
