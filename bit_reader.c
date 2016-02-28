/*
 * @file bit_reader.c
 * @author Fabjan Sukalia <fsukalia@gmail.com>
 * @date 2016-02-03
 */

#include <stdlib.h>
#include <assert.h>
#include "bit_reader.h"

struct bit_reader {
	FILE *file;
	uint8_t buffer;
	uint8_t bit_pos;
};

struct bit_reader *bit_reader_create(FILE *in)
{
	struct bit_reader *reader = calloc(1, sizeof(*reader));
	reader->file = in;
	return reader;
}

void bit_reader_destroy(struct bit_reader *reader)
{
	free(reader);
}

bool bit_reader_next_bit(struct bit_reader *reader, uint8_t *bit)
{
	assert(reader != NULL);
	assert(bit    != NULL);

	if (reader->bit_pos == 0) {
		/* read next byte */
		uint8_t data;
		if (fread(&data, sizeof(data), 1, reader->file) != sizeof(data))
			return false;

		if (data == 0xFF) {
			/* possible marker - check next byte */
			if (fread(&data, sizeof(data), 1, reader->file) != sizeof(data))
				return false;

			if (data != 0)
				return false;

			data = 0xFF;
		}

		*bit = (data & 0x80) >> 7;
		reader->buffer = data << 1;
		reader->bit_pos = 7;
		return true;
	}

	*bit = (reader->buffer & 0x80) >> 7;
	reader->buffer <<= 1;
	reader->bit_pos--;
	return true;
}

bool bit_reader_next_bits(struct bit_reader *reader, uint16_t *bits, uint8_t num)
{
	assert(reader != NULL);
	assert(bits   != NULL);
	assert(num <= 16);

	uint16_t tmp = 0;

	for (int i = 0; i < num; i++) {
		uint8_t bit;
		if (!bit_reader_next_bit(reader, &bit))
			return false;

		tmp = (tmp << 1) | (bit & 0x01);
	}

	*bits = tmp;
	return true;
}

