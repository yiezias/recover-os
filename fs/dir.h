#ifndef __FS_DIR_H
#define __FS_DIR_H
#include "inode.h"
#include "types.h"

enum file_types {
	FT_DIR,
};

#define DIRENT_SIZE 32
#define MAX_FILE_NAME_LEN 16
struct dirent {
	size_t i_no;
	char filename[MAX_FILE_NAME_LEN];
	enum file_types f_type;
};

#define DIRENT_ISNT_EXIST 	3
#define DIRENT_ALREADY_EXIST 	4
#define FILE_ISNT_EXIST 	5

ssize_t dirent_search(struct inode *dir_inode, const char *name);
ssize_t dirent_add(struct inode *dir_inode, struct dirent *de);
ssize_t dirent_delete(struct inode *dir_inode, const char *name);
struct inode *path_prase(const char *pathname, ssize_t *i_no_ptr);
#endif
