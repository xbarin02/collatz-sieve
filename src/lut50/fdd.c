#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _BSD_SOURCE
#	define fread(a,b,c,d) fread_unlocked((a),(b),(c),(d))
#	define fwrite(a,b,c,d) fwrite_unlocked((a),(b),(c),(d))
#	define feof(a) feof_unlocked((a))
#endif

typedef uint64_t word_t;

static const word_t dict[] = {
	0x0000000000000000,
	0x0000000000000080,
	0x0000000008000000,
	0x0000000008000080,
	0x0000000080000000,
	0x0000000088000000,
	0x0000008000000000,
	0x0000008000000080,
	0x0000008008000000,
	0x0000008008000080,
	0x0000008080000000,
	0x0000008088000000,
	0x0000800000000000,
	0x0000800000000080,
	0x0000800008000000,
	0x0000800008000080,
	0x0000800080000000,
	0x0000800088000000,
	0x0000808000000000,
	0x0000808000000080,
	0x0000808008000000,
	0x0000808008000080,
	0x0800000000000000,
	0x0800008000000000,
	0x0800800000000000,
	0x0800808000000000,
	0x8000000000000000,
	0x8000000000000080,
	0x8000000008000000,
	0x8000000008000080,
	0x8000000080000000,
	0x8000000088000000,
	0x8000008000000000,
	0x8000008000000080,
	0x8000008008000000,
	0x8000008008000080,
	0x8000008080000000,
	0x8000008088000000,
	0x8000800000000000,
	0x8000800000000080,
	0x8000800008000000,
	0x8000800008000080,
	0x8000808000000000,
	0x8000808000000080,
	0x8000808008000000,
	0x8000808008000080,
	0x8800000000000000,
	0x8800008000000000,
	0x8800800000000000,
	0x8800808000000000
};

static size_t size = 0;

void init_dictionary()
{
	size = sizeof dict / sizeof *dict;

	fprintf(stderr, "dictionary size = %lu\n", (unsigned long)size);
}

word_t get_word(uint8_t index)
{
	return dict[index];
}

void write_word(FILE *fout, word_t word)
{
	fwrite(&word, sizeof(word), 1, fout);
}

uint8_t read_byte(FILE *fin)
{
	uint8_t byte;

	fread(&byte, sizeof(byte), 1, fin);

	return byte;
}

static
void decompression_loop(FILE *fin, FILE *fout)
{
	uint8_t index = read_byte(fin);

	if (feof(fin)) {
		return;
	}

	write_word(fout, get_word(index));

	decompression_loop(fin, fout);
}

int main()
{
	init_dictionary();

	decompression_loop(stdin, stdout);

	if (ferror(stdin)) {
		fprintf(stderr, "input error\n");
		return 1;
	}

	if (ferror(stdout)) {
		fprintf(stderr, "output error\n");
		return 1;
	}

	return 0;
}
