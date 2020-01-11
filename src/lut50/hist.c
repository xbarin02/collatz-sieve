#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>

typedef uint64_t word_t;

word_t read_word(FILE *fin)
{
	word_t word;

	fread(&word, sizeof(word), 1, fin);

	return word;
}

static size_t hsize = 0;

struct entry {
	word_t key;
	size_t data;
};

static int cmp(const void *p1, const void *p2)
{
	const struct entry *e1 = p1;
	const struct entry *e2 = p2;

	if (e1->data > e2->data)
		return +1;

	if (e1->data < e2->data)
		return -1;

	return 0;
}

struct entry *hist = 0;

struct entry *hfind(word_t key)
{
	size_t i;

	for (i = 0; i < hsize; ++i) {
		if (hist[i].key == key) {
			return hist + i;
		}
	}

	hsize++;

	hist = realloc(hist, hsize * sizeof(struct entry));

	if (hist == NULL) {
		fprintf(stderr, "not enough memory\n");
		return NULL;
	}

	hist[hsize - 1].key = key;
	hist[hsize - 1].data = 0;

	return hist + hsize - 1;
}

int push_into_histogram(word_t word)
{
	struct entry *ep = hfind(word);

	if (ep == NULL) {
		return 1;
	} else {
		ep->data ++;
	}

	return 0;
}

int main_loop(FILE *fin)
{
	word_t word = read_word(fin);

	if (feof(fin)) {
		return 0;
	}

	if (push_into_histogram(word)) {
		return 1;
	}

	return main_loop(fin);
}

void print_histogram()
{
	size_t i;

	qsort(hist, hsize, sizeof(struct entry), cmp);

	assert(sizeof(unsigned long) == sizeof(word_t));

	for (i = 0; i < hsize; ++i) {
		printf("[%lu] 0x%016" PRIx64 " %lu (popcnt %i)\n", i, hist[i].key, hist[i].data, __builtin_popcountl((unsigned long)hist[i].key));
	}
}

int main()
{
	int errno;

	errno = main_loop(stdin);
	
	if (errno) {
		return 1;
	}

	print_histogram();

	free(hist);

	return 0;
}
