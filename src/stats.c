#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef SIEVE_LOGSIZE
#	define SIEVE_LOGSIZE 16
#endif
#define SIEVE_MASK ((1UL << SIEVE_LOGSIZE) - 1)
#define SIEVE_SIZE ((1UL << SIEVE_LOGSIZE) / 8) /* 2^k bits */
#define IS_LIVE(n) ( ( g_map_sieve[ ((n) & SIEVE_MASK)>>3 ] >> (((n) & SIEVE_MASK)&7) ) & 1 )

const void *open_map(const char *path, size_t map_size)
{
	int fd = open(path, O_RDONLY, 0600);
	void *ptr;

	if (map_size == 0) {
		map_size = 1;
	}

	if (fd < 0) {
		perror("open");
		abort();
	}

	ptr = mmap(NULL, (size_t)map_size, PROT_READ, MAP_SHARED, fd, 0);

	if (ptr == MAP_FAILED) {
		perror("mmap");
		abort();
	}

	close(fd);

	return ptr;
}

const unsigned char *g_map_sieve;

int main()
{
	char path[4096];
	size_t map_size = SIEVE_SIZE;

	uint64_t n;
	uint64_t live = 0, dead = 0;

	sprintf(path, "fsieve-%lu.map", (unsigned long)SIEVE_LOGSIZE);

	g_map_sieve = open_map(path, map_size);

	for (n = 0; n < (1UL << SIEVE_LOGSIZE); ++n) {
		if (IS_LIVE(n)) {
			live++;
		} else {
			dead++;
		}
	}

	printf("sieve k=%lu: live %lu/%lu (%f%%), dead %lu/%lu (%f%%)\n",
		(unsigned long)SIEVE_LOGSIZE,
		live, (1UL << SIEVE_LOGSIZE), live / (float)(1UL << SIEVE_LOGSIZE) * 100,
		dead, (1UL << SIEVE_LOGSIZE), dead / (float)(1UL << SIEVE_LOGSIZE) * 100
	);

	return 0;
}
