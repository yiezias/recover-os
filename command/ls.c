#include "assert.h"
#include "stdio.h"
#include "syscall.h"

int main(int argc, char *argv[]) {
	if (argc > 2) {
		printf("the number of arguments is incorrect\n");
		return -1;
	}
	const char *pathname = argc == 1 ? "/" : argv[1];
	struct stat stat_buf;
	ssize_t ret = stat((char *)pathname, &stat_buf);
	if (ret < 0 || stat_buf.file_type != FT_DIR) {
		printf("the argument is incorrect\n");
		return -1;
	}
	struct dirent *buf = (struct dirent *)brk(0);
	brk((size_t)buf + stat_buf.size);

	ssize_t fd = open(pathname);
	assert(fd >= 0);
	size_t nbyte = read(fd, buf, stat_buf.size);
	assert(nbyte == stat_buf.size);
	close(fd);
	for (size_t i = 0; i != stat_buf.size / sizeof(struct dirent); ++i) {
		printf("%s\t", buf[i].filename);
	}
	printf("\n");
	brk((size_t)buf);
	return 0;
}
