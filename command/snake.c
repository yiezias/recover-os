#include "assert.h"
#include "stdio.h"
#include "syscall.h"

enum direct {
	LEFT,
	DOWN,
	UP,
	RIGHT,
	QUIT,
};

static enum direct input(void) {
	char buf;
	read(0, &buf, 1);
	switch (buf) {
	case 'h':
		return LEFT;
	case 'j':
		return DOWN;
	case 'k':
		return UP;
	case 'l':
		return RIGHT;
	case 'q':
		return QUIT;
	default:
		return input();
	}
}

int main(void) {
	printf("snake\n");
	ssize_t pipe_fd[2] = { -1, -1 };
	pipe(pipe_fd);
	pid_t pid = clone(0, 0, NULL, NULL);
	printf("pipe down\n");
	if (pid) {
		while (1) {
			enum direct in = input();
			write(pipe_fd[0], &in, 1);
			if (in == QUIT) {
				int status;
				wait(&status);
				return 0;
			}
		}
	} else {
		enum direct in = LEFT;
		while (1) {
			read(pipe_fd[1], &in, 1);
			if (in == QUIT) {
				_exit(0);
			}
			printf("%d ", in);
		}
	}
	return 0;
}
