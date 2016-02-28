/*
 * @file encoder.c
 * @author Fabjan Sukalia <fsukalia@gmail.com>
 * @date 2016-02-21
 * @brief Tool to encode data using huffman codes in JPEG format
 */

#define _POSIX_C_SOURCE 1
#define _DEFAULT_SOURCE 

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "bit_writer.h"
#include "huff_enc.h"

#define JPG_DHT		(0xC4)

static const char *prog_name = "huffenc";

void usage(void)
{
	fprintf(stderr, "USAGE: %s FILE_IN FILE_OUT\n", prog_name);
	exit(EXIT_FAILURE);
}

static off_t get_file_size(FILE *file)
{
	int fd = fileno(file);
	if (fd == -1)
		return 0;
	
	struct stat buf;
	if (fstat(fd, &buf) != 0)
		return 0;
	
	if (S_ISREG(buf.st_mode) == 0)
		return 0;
	
	return buf.st_size;
}

void encode(FILE *in, FILE *out)
{
	off_t size = get_file_size(in);
	if (size == 0)
		return;

	uint8_t *data = malloc(size);
	if (data == NULL)
		return;

	if (fread(data, size, 1, in) != 1) {
		fprintf(stderr, "Couldn't read input data\n");
		exit(EXIT_FAILURE);
	}

	uint32_t freq[256];
	huff_get_freq(data, size, freq);

	struct huff_enc enc;
	struct huff_enc_info info;
	if (!huff_gen_enc(freq, &enc, &info)) {
		fprintf(stderr, "Couldn't create encoder\n");
		exit(EXIT_FAILURE);
	}

	uint8_t header[21];
	header[0] = 0xFF;
	header[1] = JPG_DHT;

	uint16_t header_length = 19 + info.num_codes;
	header[2] = header_length >> 8;
	header[3] = header_length & 0xFF;
	
	header[4] = 0;

	for (int i = 0; i < 16; i++) {
		header[5 + i] = info.codes_per_len[i];
	}
	
	if (fwrite(header, sizeof(header), 1, out) != 1) {
		fprintf(stderr, "Couldn't write header\n");
		exit(EXIT_FAILURE);
	}

	/* write symbols */
	for (int i = 0; i < 16; i++) {
		uint16_t num_symbols = info.codes_per_len[i];

		if (num_symbols == 0)
			continue;

		uint8_t symbols[num_symbols];

		uint16_t sym_index = 0;
		for (int j = 0; j < info.num_codes; j++) {
			if (enc.codes[j].code_len != (i + 1))
				continue;
			
			symbols[sym_index] = enc.codes[j].symbol;
			sym_index++;
		}
		
		if (fwrite(symbols, num_symbols, 1, out) != 1) {
			fprintf(stderr, "Couldn't write header\n");
			exit(EXIT_FAILURE);
		}
	}

	/* write uint32_t num_data_symbols */
	uint8_t num_bytes[4];
	num_bytes[0] = (size >> 24) & 0xFF;
	num_bytes[1] = (size >> 16) & 0xFF;
	num_bytes[2] = (size >>  8) & 0xFF;
	num_bytes[3] = size & 0xFF;

	if (fwrite(num_bytes, 4, 1, out) != 1) {
		fprintf(stderr, "Couldn't write number of bytes\n");
		exit(EXIT_FAILURE);
	}

	struct bit_writer *writer = bit_writer_create(out);
	huff_encode(&enc, size, data, writer);

	bit_writer_destroy(writer);
	huff_enc_destroy(&enc);
	free(data);
}

int main(int argc, char *argv[])
{
	errno = 0;

	if (argc > 1)
		prog_name = argv[0];
	
	if (argc != 3)
		usage();
	
	FILE *in = fopen(argv[1], "rb");
	if (in == NULL) {
		perror("Couldn't open input file");
		return EXIT_FAILURE;
	}

	FILE *out = fopen(argv[2], "wb");
	if (out == NULL) {
		perror("Couldn't open output file");
		return EXIT_FAILURE;
	}

	encode(in, out);

	fclose(in);
	fclose(out);

	return 0;
}
