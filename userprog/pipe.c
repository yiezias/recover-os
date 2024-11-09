#include "pipe.h"
#include "debug.h"
#include "file.h"
#include "ioqueue.h"
#include "task.h"

int sys_pipe(ssize_t pipe_fd[2]) {
	ssize_t ft_idx = get_free_slot(alloc_pages(1), FT_FIFO);
	if (FILE_TABLE_SIZE == ft_idx) {
		return -(int)FILE_TABLE_SIZE;
	}
	pipe_fd[0] = free_fd_alloc();
	pipe_fd[1] = free_fd_alloc();
	if (MAX_FILES_OPEN_PER_PROC == pipe_fd[0]
	    || MAX_FILES_OPEN_PER_PROC == pipe_fd[1]) {
		free_pages(file_table[ft_idx].f_inode, 1);
		return -MAX_FILES_OPEN_PER_PROC;
	}
	ioqueue_init((struct ioqueue *)file_table[ft_idx].f_inode);
	file_table[ft_idx].f_pos = 2;
	struct task_struct *task = running_task();
	task->fd_table[pipe_fd[0]] = ft_idx;
	task->fd_table[pipe_fd[1]] = ft_idx;
	return 0;
}

size_t pipe_read(size_t fd, void *buf, size_t count) {
	char *buffer = buf;
	size_t bytes_read = 0;
	size_t ft_idx = running_task()->fd_table[fd];

	struct ioqueue *ioq = (struct ioqueue *)file_table[ft_idx].f_inode;

	size_t ioq_len = ioq_length(ioq);
	size_t size = ioq_len > count ? count : ioq_len;
	while (bytes_read < size) {
		*buffer = ioq_getchar(ioq);
		++bytes_read;
		++buffer;
	}
	return bytes_read;
}

size_t pipe_write(size_t fd, const void *buf, size_t count) {
	size_t bytes_write = 0;
	uint32_t ft_idx = running_task()->fd_table[fd];
	struct ioqueue *ioq = (struct ioqueue *)file_table[ft_idx].f_inode;

	size_t ioq_left = bufsize - ioq_length(ioq);
	size_t size = ioq_left > count ? count : ioq_left;

	const char *buffer = buf;
	while (bytes_write < size) {
		ioq_putchar(ioq, *buffer);
		++bytes_write;
		++buffer;
	}
	return bytes_write;
}

ssize_t sys_dup2(size_t oldfd, size_t newfd) {
	struct task_struct *cur_task = running_task();
	size_t new_ft_idx = cur_task->fd_table[newfd];
	cur_task->fd_table[oldfd] = new_ft_idx;
	return newfd;
}
