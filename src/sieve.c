#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "wideint.h"

#ifndef SIEVE_LOGSIZE
#	define SIEVE_LOGSIZE 16
#endif

uint128_t pow3(size_t n)
{
	uint128_t r = 1;
	uint128_t b = 3;

	while (n) {
		if (n & 1) {
			assert(r <= UINT128_MAX / b);
			r *= b;
		}
		assert(b <= UINT128_MAX / b);
		b *= b;
		n >>= 1;
	}

	return r;
}

uint128_t T(uint128_t n)
{
	switch (n % 2) {
		case 0: return n / 2;
		case 1:
			assert(n <= (UINT128_MAX - 1) / 3);
			return (3 * n + 1) / 2;
	}

	return 0;
}

uint128_t T_k(uint128_t n, size_t k, size_t *o)
{
	size_t l;

	assert(o != NULL);

	*o = 0; /* odd steps */

	for (l = 0; l < k; ++l) {
		switch (n % 2) {
			case 0: /* even step */
				break;
			case 1: /* odd step */
				(*o)++;
				break;
		}

		n = T(n);
	}

	return n;
}

uint128_t pow2(size_t k)
{
	assert(k < 128);

	return UINT128_C(1) << k;
}

int is_there_lower_b_leading_to_the_same_c_d(uint128_t b, size_t k)
{
	/* this (c,d) */
	size_t c;
	uint128_t d = T_k(b, k, &c);

	/* all lower b's */
	while (--b != UINT128_MAX) {
		size_t c_;
		uint128_t d_ = T_k(b, k, &c_);

		if (c == c_ && d == d_) {
			return 1;
		}
	}

	return 0;
}

int is_killed_at_k(uint128_t b, size_t k)
{
#if 1
	size_t c;

	int128_t l, r;

	/* a 2^k + b --> a 3^c + d */
	uint128_t d = T_k(b, k, &c);
#if 1
	/* Eric's sieve */
	if (is_there_lower_b_leading_to_the_same_c_d(b, k)) {
		return 1;
	}
#endif
	assert(UINT128_C(1) <= (UINT128_MAX >> k));

	/* the question is whether "a 3^c + d < a 2^k + b" for all "a > 0" */
	/* a (3^c - 2^k) < b - d */
	/* 3^c - 2^k < b - d && 3^c - 2^k < 0 */
	l = (int128_t)pow3(c) - (int128_t)pow2(k);
	r = (int128_t)b - (int128_t)d;

	return l < r && l < 0;
#endif
#if 0
	size_t c;

	/* a 2^k + b --> a 3^c + d */
	uint128_t d = T_k(b, k, &c);

	assert(UINT128_C(1) <= (UINT128_MAX >> k));

	return pow3(c) < (UINT128_C(1) << k) && d <= b;
#endif
#if 0
	size_t c;

	/* a 2^k + b --> a 3^c + d */
	uint128_t d = T_k(b, k, &c);

	assert(UINT128_C(1) <= (UINT128_MAX >> k));

	return pow3(c) + d < (UINT128_C(1) << k) + b;
#endif
#if 0
	size_t c;

	for (; k > 0; --k) {
		/* a 2^k + b --> a 3^c + d */
		uint128_t d = T_k(b, k, &c);

		assert(UINT128_C(1) <= (UINT128_MAX >> k));

		if (pow3(c) + d < (UINT128_C(1) << k) + b)
			return 1;
	}

	return 0;
#endif
#if 0
	size_t c;

	/* a 2^k + b --> a 3^c + d */
	uint128_t d = T_k(b, k, &c);

	int128_t l, r;

	assert(UINT128_C(1) <= (UINT128_MAX >> k));

	/* a 3^c + d < a 2^k + b */
	/* a 3^c - a 2^k < b - d */
	/* a (3^c - 2^k) < (b - d) */
	/* a * l < r */
	l = (int128_t)pow3(c) - (int128_t)(UINT128_C(1) << k);
	r = (int128_t)b - (int128_t)d;

	return l < r;
#endif
}

int is_killed_below_k(uint128_t b, size_t k)
{
	while (--k > 0) {
		assert(UINT128_C(1) <= (UINT128_MAX >> k));

		if (is_killed_at_k(b % (UINT128_C(1) << k), k)) {
			return 1;
		}
	}

	return 0;
}

void *open_map(const char *path, size_t map_size)
{
	int fd = open(path, O_RDWR | O_CREAT, 0600);
	void *ptr;

	if (map_size == 0) {
		map_size = 1;
	}

	if (fd < 0) {
		perror("open");
		abort();
	}

	if (ftruncate(fd, (off_t)map_size) < 0) {
		perror("ftruncate");
		abort();
	}

	ptr = mmap(NULL, (size_t)map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (ptr == MAP_FAILED) {
		perror("mmap");
		abort();
	}

	close(fd);

	return ptr;
}

unsigned char *g_map_sieve;

#define SET_LIVE(n) ( g_map_sieve[(n)>>3] |= (1<<((n)&7)) )
#define IS_LIVE(n)  ((g_map_sieve[(n)>>3] >> ((n)&7)) & 1)

/* print number of live/dead entries */
void print_stats(size_t k)
{
	size_t b, B = pow2(k);
	size_t live = 0, dead = 0;

	for (b = 0; b < B; ++b) {
		if (IS_LIVE(b)) {
			live++;
		} else {
			dead++;
		}
	}

	printf("sieve-%lu: live %lu/%lu dead %lu/%lu\n", k, live, B, dead, B);
}

int main()
{
	char path[4096];
	size_t k = SIEVE_LOGSIZE;
	uint128_t b;
	size_t map_size = ((size_t)1 << k) / 8; /* 2^k bits */

	sprintf(path, "sieve-%lu.map", (unsigned long)k);

	g_map_sieve = open_map(path, map_size);

	/* a 2^k + b */
	for (b = 0; b < (UINT128_C(1) << k); ++b) {
		int killed = is_killed_below_k(b, k) || is_killed_at_k(b, k);

		if (!killed) {
			SET_LIVE(b);
		}
	}

	print_stats(k);

	msync(g_map_sieve, map_size, MS_SYNC);
	munmap(g_map_sieve, map_size);

	return 0;
}
