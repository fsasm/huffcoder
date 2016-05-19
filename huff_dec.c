/*
 * @file huff_dec.c
 * @author Fabjan Sukalia <fsukalia@gmail.com>
 * @date 2016-02-03
 */

#include <stdlib.h>
#include <assert.h>
#include "bit_reader.h"
#include "huff_dec.h"

bool huff_gen_dec(uint8_t code_len[restrict 16], uint8_t symbols[restrict],
				  struct huff_dec * restrict decoder)
{
	assert(code_len != NULL);
	assert(symbols  != NULL);
	assert(decoder  != NULL);

	uint16_t num_sym = 0;
	uint8_t max_bits = 0;
	uint8_t min_bits = 0;

	for (int i = 0; i < 16; i++) {
		num_sym += code_len[i];
		max_bits = (code_len[i] != 0) ? i + 1 : max_bits;
		min_bits = (min_bits == 0) ? i + 1 : min_bits;
	}

	if (num_sym > 256 && num_sym == 0) {
		fprintf(stderr, "Invalid number of symbols\n");
		return false;
	}

	assert(min_bits <= max_bits);
	assert(0 < max_bits && max_bits <= 16);
	assert(0 < min_bits && min_bits <= 16);

	uint32_t num_entries = 1 << max_bits;
	assert(num_entries != 0);

	decoder->max_bits = max_bits;
	decoder->min_bits = min_bits;
	decoder->entries  = malloc(sizeof(uint16_t) * num_entries);

	if(decoder->entries == NULL) {
		perror("Couldn't allocate memory for decode table\n");
		return false;
	}

	uint8_t sym_index = 0;
	size_t index = 0;

	uint32_t times = 1 << max_bits;
	for (int i = 0; i < 16; i++) {
		times >>= 1;

		for (int j = 0; j < code_len[i]; j++) {
			uint8_t symbol = symbols[sym_index];
			sym_index++;

			for (uint16_t t = 0; t < times; t++) {
				decoder->entries[index] = (symbol << 8) | (i + 1);
				index++;
				assert(index <= num_entries);
			}
		}
	}

	/* if the kraft sum is less than 1 then index is less than num_entries
	 * this isn't really an error, there are just some checks in the decoding 
	 * missing. */
	if (index != num_entries) {
		fprintf(stderr, "Invalid decode header. Missing entries in decode table\n");
		free(decoder->entries);
		return false;
	}

	return true;
}

bool huff_decode(const struct huff_dec * restrict decoder, size_t num_sym,
				 struct bit_reader * restrict reader, 
				 uint8_t out_buf[restrict])
{
	assert(decoder != NULL);
	assert(reader  != NULL);
	assert(out_buf != NULL);

	if (num_sym == 0)
		return true;
	
	/* FIXME this only works when decoding starts at byte boundary. Add functions
	   to push back bits in bit_reader and extend decoding to decode at any bit
	   position. */
	
	uint16_t code;
	uint8_t max_bits = decoder->max_bits;
	const uint16_t mask = (1 << max_bits) - 1;
	uint16_t *table = decoder->entries;

	/* FIXME special case with less than max_bits in the data stream. */
	if (!bit_reader_next_bits(reader, &code, max_bits)) {
		fprintf(stderr, "Error while reading input\n");
		return false;
	}

	for (size_t i = 0; i < num_sym; i++) {
		code &= mask;
		uint16_t entry = table[code];
		uint8_t symbol = (entry >> 8) & 0xFF;
		uint8_t length = entry & 0xFF;

		assert(length != 0);
		assert(length <= max_bits);

		out_buf[i] = symbol;

		if (i == num_sym - 1)
			break;

		uint16_t tmp = 0;
		if (!bit_reader_next_bits(reader, &tmp, length)) {
			fprintf(stderr, "Error while reading input\n");
			return false;
		}
		
		code = (code << length) | tmp;
	}

	return true;
}

bool huff_decode_file(const struct huff_dec * restrict decoder, size_t num_sym,
					  struct bit_reader * restrict reader, FILE *out)
{
	assert(decoder != NULL);
	assert(reader  != NULL);
	assert(out     != NULL);

	if (num_sym == 0)
		return true;
	
	/* FIXME this only works when decoding starts at byte boundary. Add functions
	   to push back bits in bit_reader and extend decoding to decode at any bit
	   position. */
	
	uint32_t code;
	uint8_t max_bits = decoder->max_bits;
	const uint16_t mask = (1 << max_bits) - 1;
	uint16_t *table = decoder->entries;

	/* FIXME special case with less than max_bits in the data stream. */
	/* FIXME handle EOF correctly */
	uint16_t code16;
	bit_reader_next_bits(reader, &code16, max_bits);
	code = code16;
	/*
		fprintf(stderr, "Error while reading input\n");
		return false;
	}*/

	for (size_t i = 0; i < num_sym; i++) {
		code &= mask;
		uint16_t entry = table[code];
		uint8_t symbol = (entry >> 8) & 0xFF;
		uint8_t length = entry & 0xFF;

		assert(length != 0);
		assert(length <= max_bits);
		
		if (fwrite(&symbol, 1, 1, out) != 1) {
			fprintf(stderr, "Error while writing output symbols\n");
			return false;
		}
		
		if (i == num_sym - 1)
			break;

		uint16_t tmp = 0;
		bit_reader_next_bits(reader, &tmp, length);/*
			fprintf(stderr, "Error while reading input\n");
			return false;
		}*/
		
		/* FIXME this can overflow when code is uint16_t */
		code = (code << length) | tmp;
	}

	return true;
}

void huff_destroy(struct huff_dec *dec)
{
	assert(dec != NULL);
	free(dec->entries);
}

