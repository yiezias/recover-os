#include "wait_exit.h"
#include "debug.h"
#include "file.h"
#include "global.h"
#include "memory.h"

static bool find_hanging_child(struct list_elem *elem, void *parent_task) {
	struct task_struct *ptask =
		elem2entry(struct task_struct, all_list_tag, elem);
	if (ptask->parent_task == parent_task
	    && ptask->status == TASK_HANGING) {
		return true;
	}
	return false;
}

static bool find_child(struct list_elem *elem, void *parent_task) {
	struct task_struct *ptask =
		elem2entry(struct task_struct, all_list_tag, elem);
	if (ptask->parent_task == parent_task) {
		return true;
	}
	return false;
}

pid_t sys_wait(int *status) {
	struct task_struct *task = running_task();

	while (1) {
		struct list_elem *child_elem = list_traversal(
			&all_tasks_list, find_hanging_child, task);
		if (child_elem != NULL) {
			struct task_struct *child_task = elem2entry(
				struct task_struct, all_list_tag, child_elem);
			*status = child_task->exit_status;
			pid_t child_pid = child_task->pid;

			child_task->status = TASK_DIED;
			if (task->pml4 != child_task->pml4) {
				free_pages(child_task->pml4, 1);
			}
			if (elem_find(&ready_tasks_list,
				      &child_task->general_tag)) {
				list_remove(&child_task->general_tag);
			}
			list_remove(&child_task->all_list_tag);
			release_pid(child_pid);
			free_pages(child_task, 1);
			return child_pid;
		}
		child_elem = list_traversal(&all_tasks_list, find_child, task);
		if (child_elem == NULL) {
			return -1;
		} else {
			task_block(TASK_WAITING);
		}
	}
}


static bool init_adopt_a_child(struct list_elem *pelem, void *task) {
	struct task_struct *ptask =
		elem2entry(struct task_struct, all_list_tag, pelem);
	if (ptask->parent_task == task) {
		ptask->parent_task = init_proc;
	}
	return false;
}

static void release_prog_resource(struct task_struct *release_task) {
	sys_brk(release_task->addr_space.heap_start);
	for (int i = 0; i != 4; ++i) {
		/* 应当把四级页表全部释放 */
		size_t vaddr = release_task->addr_space.vaddr[i];
		size_t filesz = release_task->addr_space.filesz[i];
		if (vaddr == 0) {
			continue;
		}
		size_t page_start = vaddr & (~0xfff);
		size_t page_end =
			PG_SIZE * DIV_ROUND_UP((vaddr + filesz), PG_SIZE);
		size_t pg_cnt = (page_end - page_start) / PG_SIZE;
		for (size_t i = 0; i != pg_cnt; ++i) {
			page_unmap(page_start + i * PG_SIZE);
		}
	}
	page_unmap(DEFAULT_STACK - PG_SIZE);

	for (int fd_idx = 0; fd_idx != MAX_FILES_OPEN_PER_PROC; ++fd_idx) {
		if (release_task->fd_table[fd_idx] != -1) {
			sys_close(fd_idx);
		}
	}
}

void sys_exit(int status) {
	struct task_struct *task = running_task();
	task->exit_status = status;
	ASSERT(task->pid > 1);

	list_traversal(&all_tasks_list, init_adopt_a_child, task);
	/* 删除进程资源 */
	release_prog_resource(task);

	if (task->parent_task->status == TASK_WAITING) {
		task_unblock(task->parent_task);
	}
	task_block(TASK_HANGING);
}
