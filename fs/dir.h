#ifndef __FS_DIR_H
#define __FS_DIR_H
#include "inode.h"
#include "types.h"

enum file_types {
	FT_DIR,
	FT_CHR,
	FT_REG,
	FT_FIFO,
};

#define DIRENT_SIZE 32
#define MAX_FILE_NAME_LEN 16
struct dirent {
	size_t i_no;
	char filename[MAX_FILE_NAME_LEN];
	enum file_types f_type;
};

struct stat {
	size_t ino;
	enum file_types file_type;
	size_t size;
};

#define DIRENT_ISNT_EXIST 3
#define DIRENT_ALREADY_EXIST 4
#define FILE_ISNT_EXIST 5
#define ISNT_DIR 6
#define DIR_ISNT_EMPTY 7

ssize_t dirent_search(struct inode *dir_inode, const char *name);
ssize_t dirent_add(struct inode *dir_inode, struct dirent *de);
ssize_t dirent_delete(struct inode *dir_inode, const char *name);
ssize_t path_prase(const char *pathname, struct dirent *de_buf);
ssize_t sys_mkdir(char *pathname);
ssize_t sys_rmdir(char *pathname);
ssize_t sys_stat(char *pathname, struct stat *stat_buf);
#endif
