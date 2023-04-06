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

#define K SIEVE_LOGSIZE

uint128_t pow2(size_t k)
{
	assert(k < 128);

	return UINT128_C(1) << k;
}

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

#define  IS_LIVE(k, n) ((g_map_sieve[(k)][(n)>>3] >> ((n)&7)) & 1)
#define SET_LIVE(k, n) ( g_map_sieve[(k)][(n)>>3] |= (1<<((n)&7)) )
#define SET_DEAD(k, n) ( g_map_sieve[(k)][(n)>>3] &= UCHAR_MAX ^ (1<<((n)&7)) )

/* sieve[k-index][bit-index] */
unsigned char *g_map_sieve[K+1];

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

/* the question is whether "a 3^c + d < a 2^k + b" for all "a > 0" */
int is_convergent(size_t k, size_t b, size_t c, uint128_t d)
{
	int128_t lhs, rhs;

	assert(pow3(k) <= (uint128_t)INT128_MAX);
	assert(pow2(k) <= (uint128_t)INT128_MAX);
	assert((int128_t)pow3(c) >= INT128_MIN + (int128_t)pow2(k));

	/* a (3^c - 2^k) < b - d */
	/* 3^c - 2^k < b - d && 3^c - 2^k < 0 */

	lhs = (int128_t)pow3(c) - (int128_t)pow2(k);

	assert(sizeof(size_t) < sizeof(int128_t));
	assert(d <= (uint128_t)INT128_MAX);
	assert((int128_t)b >= INT128_MIN + (int128_t)d);

	rhs = (int128_t)b - (int128_t)d;

	/* printf("a*%lu + %lu < a*2^%lu + %lu [%s]\n", (uint64_t)pow3(c), (uint64_t)d, (uint64_t)k, (uint64_t)b, (lhs < rhs && lhs < 0) ? "DEAD" : "LIVE"); */

	return lhs < rhs && lhs < 0;
}

/* Eric's idea */
int join_lower_trajectory(size_t b, size_t *c, uint128_t *d)
{
	size_t C = c[b];
	size_t D = d[b];

	while (--b != (size_t)-1) {
		if (C == c[b] && D == d[b]) {
			return 1;
		}
	}

	return 0;
}

#define PACK_STRUCT_ELEMENTS

#define PACK_STRUCT_ELEMENTS_ASYM

struct elem {
#ifdef PACK_STRUCT_ELEMENTS
	uint64_t d;
#	ifdef PACK_STRUCT_ELEMENTS_ASYM
	uint64_t bc;
#	else
	uint32_t b;
	uint32_t c;
#	endif
#else
	uint128_t d;
	size_t b;
	size_t c;
#endif
};

struct elem pack(size_t c, uint128_t d, size_t b)
{
	struct elem e;

#ifdef PACK_STRUCT_ELEMENTS
	assert(d < (UINT128_C(1) << 64));
	assert(sizeof(size_t) >= sizeof(uint64_t));

	e.d = (uint64_t)d;
#	ifdef PACK_STRUCT_ELEMENTS_ASYM
	assert(b < ((size_t)1 << 34));
	assert(c < ((size_t)1 << 30));

	/* bc = [ b : 34 | c : 30 ] */
	e.bc = ((uint64_t)b << 30) | (uint64_t)c;
#	else
	assert(b < ((size_t)1 << 32));
	assert(c < ((size_t)1 << 32));

	e.b = (uint32_t)b;
	e.c = (uint32_t)c;
#	endif
#else
	e.d = d;
	e.b = b;
	e.c = c;
#endif

	return e;
}

#define M30 ((UINT64_C(1) << 30) - 1)
#define M34 ((UINT64_C(1) << 34) - 1)

size_t unpack_c(const struct elem *elem)
{
	assert(elem != NULL);

#ifdef PACK_STRUCT_ELEMENTS
#	ifdef PACK_STRUCT_ELEMENTS_ASYM
	/* bc = [ b : 34 | c : 30 ] */
	return (size_t)(elem->bc & M30);
#	else
	return (size_t)elem->c;
#	endif
#else
	return elem->c;
#endif
}

size_t unpack_b(const struct elem *elem)
{
	assert(elem != NULL);

#ifdef PACK_STRUCT_ELEMENTS
#	ifdef PACK_STRUCT_ELEMENTS_ASYM
	/* bc = [ b : 34 | c : 30 ] */
	return (size_t)((elem->bc >> 30) & M34);
#	else
	return (size_t)elem->b;
#	endif
#else
	return elem->b;
#endif
}

uint128_t unpack_d(const struct elem *elem)
{
	assert(elem != NULL);

#ifdef PACK_STRUCT_ELEMENTS
	return (uint128_t)elem->d;
#else
	return elem->d;
#endif
}

int compar(const void *p0, const void *p1)
{
	const struct elem *e0 = (const struct elem *)p0;
	const struct elem *e1 = (const struct elem *)p1;

	if (unpack_c(e0) < unpack_c(e1))
		return -1;
	if (unpack_c(e0) > unpack_c(e1))
		return +1;

	if (unpack_d(e0) < unpack_d(e1))
		return -1;
	if (unpack_d(e0) > unpack_d(e1))
		return +1;

	if (unpack_b(e0) < unpack_b(e1))
		return -1;
	if (unpack_b(e0) > unpack_b(e1))
		return +1;

	return 0;
}

#define SAVE_MEMORY
#define EARLY_TERM

/* generate sieve[k], all k' < k are already available */
int generate_sieve(size_t k)
{
	size_t b, B = pow2(k);

#ifndef SAVE_MEMORY
	size_t *c = malloc(sizeof(size_t) * B);
	uint128_t *d = malloc(sizeof(uint128_t) * B);
#endif

	struct elem *e = malloc(sizeof(struct elem) * B);

	if (
#ifndef SAVE_MEMORY
	    c == NULL || d == NULL ||
#endif
	    e == NULL) {
		perror("malloc");

#ifndef SAVE_MEMORY
		free(c);
		free(d);
#endif
		free(e);

		return -1;
	}

	/* for each b in [0..2^k) */
	#pragma omp parallel for
	for (b = 0; b < B; ++b) {
		/* a 2^k + b --> a 3^c + d */
#ifndef SAVE_MEMORY
		d[b] = T_k((uint128_t)b, k, c+b);
#else
		size_t c_;
		uint128_t d_ = T_k((uint128_t)b, k, &c_);
#endif

#ifndef SAVE_MEMORY
		e[b] = pack(c[b], d[b], b);
#else
		e[b] = pack(c_, d_, b);
#endif

		/* become dead at this k? */
#ifndef SAVE_MEMORY
		if (is_convergent(k, b, c[b], d[b])) {
#else
		if (is_convergent(k, b, c_, /*c_*/d_)) {
#endif
#ifndef _OPENMP
			SET_DEAD(k, b);
#else
			#pragma omp atomic
			g_map_sieve[k][(b)>>3] &= UCHAR_MAX ^ (1<<((b)&7));
#endif
		}

		/* was already dead in k-1? */
		if (k > 0) {
			if (!IS_LIVE(k-1, b & (pow2(k-1)-1))) {
#ifndef _OPENMP
				SET_DEAD(k, b);
#else
				#pragma omp atomic
				g_map_sieve[k][(b)>>3] &= UCHAR_MAX ^ (1<<((b)&7));
#endif
			}
		}
	}
#if 1 /* Eric's join a path of lower number */
	qsort(e, B, sizeof(struct elem), compar);

	#pragma omp parallel for
	for (b = 0; b < B; ++b) {
		/* join a path of lower number? */
#if 0
#	ifdef EARLY_TERM
		if (!IS_LIVE(k, b))
			continue;
#	endif
		if (join_lower_trajectory(b, c, d)) {
			SET_DEAD(k, b);
		}
#else
		struct elem f, *r;

#	ifdef SAVE_MEMORY
		size_t c_;
		uint128_t d_;
#	endif

#	ifdef EARLY_TERM
		if (!IS_LIVE(k, b))
			continue;
#	endif

#	ifndef SAVE_MEMORY
		f = pack(c[b], d[b], b);
#	else
		d_ = T_k((uint128_t)b, k, &c_);
		f = pack(c_, d_, b);
#	endif

		r = bsearch(&f, e, B, sizeof(struct elem), compar);

		assert(r != NULL);

		assert(unpack_b(r) == unpack_b(&f));

#	ifdef EARLY_TERM
		if (r >= e + 1) {
#	else
		if (r + 1 < e + B) {
#	endif
#	ifdef EARLY_TERM
			const struct elem *s = r - 1;
#	else
			const struct elem *s = r + 1;
#	endif
			if (unpack_c(s) == unpack_c(r) && unpack_d(s) == unpack_d(r)) {
#	ifdef EARLY_TERM
				assert(unpack_b(s) < unpack_b(r));
#	else
				assert(unpack_b(s) > unpack_b(r));
#	endif

#	ifndef _OPENMP
#		ifdef EARLY_TERM
				SET_DEAD(k, b);
#		else
				SET_DEAD(k, (size_t)unpack_b(s));
#		endif
#	else
#		ifdef EARLY_TERM
				#pragma omp atomic
				g_map_sieve[k][(b)>>3] &= UCHAR_MAX ^ (1<<((b)&7));
#		else
				#pragma omp atomic
				g_map_sieve[k][((size_t)unpack_b(s))>>3] &= UCHAR_MAX ^ (1<<(((size_t)unpack_b(s))&7));
#		endif
#	endif
			}
		}
#endif
	}
#endif

#ifndef SAVE_MEMORY
	free(c);
	free(d);
#endif
	free(e);

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

	ptr = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (ptr == MAP_FAILED) {
		perror("mmap");
		abort();
	}

	close(fd);

	return ptr;
}

void close_map(void *ptr, size_t map_size)
{
	msync(ptr, map_size, MS_SYNC);

	munmap(ptr, map_size);
}

void *open_sieve(size_t k)
{
	/* 2^k bits */
	size_t map_size = ((size_t)1 << k) / 8;
	char path[4096];

	sprintf(path, "fsieve-%lu.map", (unsigned long)k);

	return open_map(path, map_size);
}

void close_sieve(void *ptr, size_t k)
{
	/* 2^k bits */
	size_t map_size = ((size_t)1 << k) / 8;

	close_map(ptr, map_size);
}

/* mask all bit-indices live by default */
void init_sieve(size_t k)
{
	size_t b, B = pow2(k);

	for (b = 0; b < B; ++b) {
		SET_LIVE(k, b);
	}
}

/* print number of live/dead entries */
void print_stats(size_t k)
{
	size_t b, B = pow2(k);
	size_t live = 0, dead = 0;

	for (b = 0; b < B; ++b) {
		if (IS_LIVE(k, b)) {
			live++;
		} else {
			dead++;
		}
	}

	printf("sieve-%lu: live %lu/%lu dead %lu/%lu\n", k, live, B, dead, B);
}

void export(size_t k)
{
	size_t b, B = pow2(k);
	char path[4096];
	FILE *fdead, *flive;

	sprintf(path, "dead-%lu.txt", (unsigned long)k);
	fdead = fopen(path, "w");

	sprintf(path, "live-%lu.txt", (unsigned long)k);
	flive = fopen(path, "w");

	if (fdead == NULL) {
		abort();
	}

	if (flive == NULL) {
		abort();
	}

	for (b = 0; b < B; ++b) {
		if (IS_LIVE(k, b)) {
			/* live */
			fprintf(flive, "%lu\n", (unsigned long)b);
		} else {
			/* dead */
			fprintf(fdead, "%lu\n", (unsigned long)b);
		}
	}

	fclose(fdead);
	fclose(flive);
}

int main()
{
	size_t k;

	printf("sizeof(struct elem) = %lu\n", (unsigned long)sizeof(struct elem));

	/* target K-sieve: generate all k < K+1 bit sieves */
	/* NOTE that the loop includes K */
	for (k = 0; k < K + 1; ++k) {
		g_map_sieve[k] = open_sieve(k);

		init_sieve(k);

		if (generate_sieve(k) < 0) {
			abort();
		}

		print_stats(k);
#if 0
		export(k);
#endif
	}

	for (k = 0; k < K + 1; ++k) {
		close_sieve(g_map_sieve[k], k);
	}

	return 0;
}
