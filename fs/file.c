#include "file.h"
#include "debug.h"
#include "dir.h"
#include "global.h"
#include "task.h"

struct file *file_table;
struct semaphore file_table_lock;

static ssize_t get_free_slot(struct inode *inode, enum file_mode mode) {
	ssize_t i = 0;
	for (; i != FILE_TABLE_SIZE; ++i) {
		if (file_table[i].f_inode == NULL) {
			break;
		}
	}
	if (i != FILE_TABLE_SIZE) {
		file_table[i].f_inode = inode;
		file_table[i].f_pos = 0;
		file_table[i].f_mode = mode;
	}
	return i;
}

static ssize_t free_fd_alloc(void) {
	struct task_struct *cur_task = running_task();
	ssize_t i = 0;
	for (; i != MAX_FILES_OPEN_PER_PROC; ++i) {
		if (cur_task->fd_table[i] == -1) {
			break;
		}
	}
	return i;
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
	/* 文件权限判断 */
	enum file_mode mode = 0;
	if (de_buf.f_type == FT_DIR) {
		mode = readable;
	}
	/* 从文件表中找空闲位 */
	sema_down(&file_table_lock);
	ssize_t ft_idx = get_free_slot(inode_open(de_buf.i_no), mode);
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
	inode_close(file_table[ft_idx].f_inode);
	file_table[ft_idx].f_inode = NULL;
	sema_up(&file_table_lock);
	/* 置进程文件描述符为空闲 */
	cur_task->fd_table[fd] = -1;
	return 0;
}
