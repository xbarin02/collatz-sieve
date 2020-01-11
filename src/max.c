#include <stdio.h>
#ifdef _USE_GMP
#	include <gmp.h>
#endif
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>
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

const unsigned char *g_map_sieve;

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

void init()
{
	char path[4096];
	size_t k = SIEVE_LOGSIZE;
	size_t map_size = SIEVE_SIZE;

	sprintf(path, "sieve-%lu.map", (unsigned long)k);

	g_map_sieve = open_map(path, map_size);
}

uint64_t g_max = 0;

uint64_t T(uint64_t n)
{
	switch (n % 2) {
		case 0: return n / 2;
		case 1: return (3 * n + 1) / 2;
	}

	return 0;
}

static int ctz(uint64_t n)
{
	assert(sizeof(uint64_t) == sizeof(unsigned long));
	if (n == 0) {
		return 64;
	}
	return __builtin_ctzl((unsigned long)n);
}

uint64_t pow3(uint64_t n)
{
	uint64_t r = 1;
	uint64_t b = 3;

	assert(n < 41);

	while (n) {
		if (n & 1) {
			r *= b;
		}
		b *= b;
		n >>= 1;
	}

	return r;
}

uint64_t Tf(uint64_t n)
{
	int alpha;

	n >>= ctz(n); /* beta */

	n++;
	alpha = ctz(n);
	n >>= alpha;
	assert(n <= UINT64_MAX >> 2*alpha);
	n *= pow3(alpha);
	n--;

	return n;
}

void printf_path(uint64_t n0)
{
	uint64_t n = n0;

	printf("%lu", n0);

	while (n >= n0 && n != 1 && n != 2) {
		n = Tf(n);
#if 0
		printf(" -> %lu", n);
#endif
		if (n > g_max) {
			printf(" [MAX:%lu]", n);
			g_max = n;
		}
	}
}

static int is_live_in_sieve_3(uint64_t n)
{
#if 1
	(void)n;
	return 1;
#else
	return (n % 3) != 2;
#endif
}

int main()
{
	uint64_t n;

	init();

	for (n = 1; ; ++n) {
		if (IS_LIVE(n) && is_live_in_sieve_3(n)) {
			printf("[LIVE] ");
		} else {
			printf("[DEAD] ");
		}

		printf_path(n);

		printf("\n");
	}

	return 0;
}
