#include "assert.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"

#define HEI 20
#define WID 20

enum status {
	SPACE,
	BLOCK,
};

static void output(enum status *map, size_t fsite) {
	printf("\f%ld", fsite);
	for (size_t idx = 0; idx != WID * HEI; ++idx) {
		if (idx % WID == 0) {
			printf("\n");
		}
		if (idx == fsite) {
			printf("()");
		} else if (map[idx]) {
			printf("[]");
		} else {
			printf("  ");
		}
	}
	return;
}

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

size_t seed = 1804289383;
static void srand(size_t nseed) {
	seed = nseed;
}

static size_t rand(void) {
	seed = (seed * 6807) % 2147483647;
	return seed;
}

static void sleep(size_t m_sec) {
	size_t tick = clock_gettime(1);
	while (clock_gettime(1) - tick < m_sec) {
		sched_yield();
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
		enum status map[WID * HEI] = { 0 };
		size_t sites[WID * HEI], start = 0, end = 0, fsite = 0;
		size_t site = (HEI / 2) * WID + WID / 2;
		memset(map, 0, sizeof(enum status) * WID * HEI);
		memset(map, 0, sizeof(int) * WID * HEI);
		for (int i = 0; i < WID * HEI; i++) {
			map[i] = !(i / WID % (HEI - 1) && i % WID % (WID - 1));
		}
		map[sites[start = (start + 1) % (WID * HEI)] = site] = BLOCK;
		srand(clock_gettime(1));
		do {
			fsite = rand() % (WID * HEI);
		} while (map[fsite]);
		while (1) {
			read(pipe_fd[1], &in, 1);
			if (in == QUIT) {
				break;
			}
			size_t dist = (in % 2 ? 1 : -1)
				      * (in == UP || in == DOWN ? WID : 1);
			if (map[site += dist]) {
				printf("\nthe snake is hit\n");
				break;
			}
			map[sites[start = (start + 1) % (WID * HEI)] = site] =
				BLOCK;
			if (site == fsite) {
				do {
					fsite = rand() % (WID * HEI);
				} while (map[fsite]);
			} else {
				map[sites[end = (end + 1) % (WID * HEI)]] =
					SPACE;
			}
			output(map, fsite);
			sleep(100000);
		}
		_exit(0);
	}
	return 0;
}
