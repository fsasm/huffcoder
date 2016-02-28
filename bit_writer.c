/*
 * @file bit_writer.c
 * @author Fabjan Sukalia <fsukalia@gmail.com>
 * @date 2016-02-16
 */

#include <stdlib.h>
#include "bit_writer.h"

struct bit_writer {
	FILE *file;
	uint8_t buffer;
	uint8_t bit_pos;
};

struct bit_writer *bit_writer_create(FILE *out)
{
	struct bit_writer *writer = calloc(1, sizeof(*writer));
	writer->file = out;
	return writer;
}

void bit_writer_destroy(struct bit_writer *writer)
{
	/* write out remaining bits */
	if (writer->bit_pos > 0) {
		/* append '1' */
		while(writer->bit_pos != 0)
			bit_writer_next_bit(writer, 1);
	}

	free(writer);
}

bool bit_writer_next_bit(struct bit_writer *writer, uint8_t bit)
{
	uint8_t buffer = writer->buffer;
	uint8_t bit_pos = writer->bit_pos;

	buffer |= (bit & 0x01) << (7 - bit_pos);
	bit_pos++;

	if (bit_pos == 8) {
		if (fwrite(&buffer, sizeof(buffer), 1, writer->file) != 1)
			return false;

		if (buffer == 0xFF) {
			buffer = 0;
			if (fwrite(&buffer, sizeof(buffer), 1, writer->file) != 1)
				return false;
		}
		bit_pos = 0;
		buffer = 0;
	}

	writer->buffer = buffer;
	writer->bit_pos = bit_pos;
	return true;
}

bool bit_writer_next_bits(struct bit_writer *writer, uint16_t bits, uint8_t num)
{
	/* Msb first */
	for (int i = 0; i < num; i++) {
		uint8_t bit = (bits >> (num - i - 1));
		if (!bit_writer_next_bit(writer, bit))
			return false;

	}

	return true;
}

#if 0

int main(void)
{
	FILE *out = fopen("test1.bin", "wr");

	struct bit_writer *writer = bit_writer_create(out);
	bit_writer_next_bits(writer, 0x04, 3); // H
	bit_writer_next_bits(writer, 0x0A, 4); // e
	bit_writer_next_bits(writer, 0x00, 2); // l
	bit_writer_next_bits(writer, 0x00, 2); // l
	bit_writer_next_bits(writer, 0x01, 2); // o
	bit_writer_next_bits(writer, 0x0B, 4); // SPACE
	bit_writer_next_bits(writer, 0x0C, 4); // W
	bit_writer_next_bits(writer, 0x01, 2); // o
	bit_writer_next_bits(writer, 0x0D, 4); // r
	bit_writer_next_bits(writer, 0x00, 2); // l
	bit_writer_next_bits(writer, 0x0E, 4); // d
	bit_writer_next_bits(writer, 0x0F, 4); // \n
	bit_writer_destroy(writer);
}
#endif 

