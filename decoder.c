/*
 * decoder.c
 * @date 2016-02-03
 * @author Fabjan Sukalia <fsukalia@gmail.com>
 * @brief Tool to decode huffman encoded data in JPEG format
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "bit_reader.h"
#include "huff_dec.h"

#define JPG_DHT		(0xC4)

static const char *prog_name = "hufdec";

void usage(void)
{
	fprintf(stderr, "USAGE: %s FILE_IN [FILE_OUT]\n", prog_name);
	exit(EXIT_FAILURE);
}


void decode(FILE *in, FILE *out)
{
	uint8_t header[21];
	if (fread(header, sizeof(header), 1, in) != 1) {
		fprintf(stderr, "Couldn't read header\n");
		exit(EXIT_FAILURE);
	}

	if (header[0] != 0xFF || header[1] != JPG_DHT) {
		fprintf(stderr, "Invalid header\n");
		exit(EXIT_FAILURE);
	}

	uint16_t header_length = (header[2] << 8) | header[3];
	if (header_length < 19 || header_length > (19 + 256)) {
		fprintf(stderr, "Invalid header length\n");
		exit(EXIT_FAILURE);
	}

	/* table class and destination index - must be zero */
	if (header[4] != 0) {
		fprintf(stderr, "Invalid table class and destination index\n");
		exit(EXIT_FAILURE);
	}

	uint16_t sum_symbol = 0;
	for (int i = 0; i < 16; i++)
		sum_symbol += header[5 + i];

	if (sum_symbol == 0 || header_length == 19)
		return; /* nothing to do */

	uint8_t symbols[sum_symbol];
	if (fread(symbols, sizeof(symbols), 1, in) != 1) {
		fprintf(stderr, "Couldn't read symbol table\n");
		exit(EXIT_FAILURE);
	}

	/* how many bytes for the output or how many symbols to read */
	uint8_t tmp[4];
	if (fread(tmp, 4, 1, in) != 1) {
		fprintf(stderr, "Couldn't read number of data\n");
		exit(EXIT_FAILURE);
	}

	uint32_t num_sym = (tmp[0] << 24) | (tmp[1] << 16) | (tmp[2] << 8) | tmp[3];

	struct huff_dec dec;
	if (!huff_gen_dec(&header[5], symbols, &dec)) {
		fprintf(stderr, "Couldn't create decoder\n");
		exit(EXIT_FAILURE);
	}
	struct bit_reader *reader = bit_reader_create(in);

	if (reader == NULL) {
		fprintf(stderr, "Couldn't create bit reader\n");
		exit(EXIT_FAILURE);
	}

	if (!huff_decode_file(&dec, num_sym, reader, out)) {
		fprintf(stderr, "Error while decoding\n");
		exit(EXIT_FAILURE);
	}

	bit_reader_destroy(reader);
	huff_destroy(&dec);
}

int main(int argc, char *argv[])
{
	errno = 0;

	if (argc > 1)
		prog_name = argv[0];
	
	if (argc != 3 && argc != 2)
		usage();
	
	FILE *in = fopen(argv[1], "rb");
	if (in == NULL) {
		perror("Couldn't open input file");
		return EXIT_FAILURE;
	}

	FILE *out;
	if (argc == 3) {
		out = fopen(argv[2], "wb");
		if (out == NULL) {
			perror("Couldn't open output file");
			return EXIT_FAILURE;
		}
	} else {
		out = stdout;
	}

	decode(in, out);

	fclose(in);
	fclose(out);

	return 0;
}

