/*
 * @file huff_dec.c
 * @author Fabjan Sukalia <fsukalia@gmail.com>
 * @date 2016-02-03
 */

#include <stdlib.h>
#include <assert.h>
#include "bit_reader.h"
#include "huff_dec.h"

struct huff_dec {
	uint8_t max_bits; /* num_entries = 1 << num_bits; */
	uint8_t min_bits;

	/* high byte: symbol; low byte: num_bits; invalid code if num_bits = 0 */
	uint16_t *entries;
};

struct huff_dec *huff_gen_dec(uint8_t code_len[restrict 16], uint8_t symbols[restrict])
{
	assert(code_len != NULL);
	assert(symbols  != NULL);

	uint8_t num_sym = 0;
	uint8_t max_bits = 0;
	uint8_t min_bits = 0;

	for (int i = 0; i < 16; i++) {
		num_sym += code_len[i];
		max_bits = (code_len[i] != 0) ? i + 1 : max_bits;
		min_bits = (min_bits == 0 && code_len[i] != 0) ? i + 1 : min_bits;
	}

	uint8_t num_entries = 1 << max_bits;

	struct huff_dec *dec = calloc(1, sizeof(*dec));
	dec->max_bits = max_bits;
	dec->min_bits = min_bits;
	dec->entries = malloc(sizeof(uint16_t) * num_entries);

	uint8_t sym_index = 0;
	size_t index = 0;

	uint16_t times = 1 << max_bits;
	for (int i = 0; i < 16; i++) {
		times >>= 1;

		for (int j = 0; j < code_len[i]; j++) {
			uint8_t symbol = symbols[sym_index];
			sym_index++;

			for (uint16_t t = 0; t < times; t++) {
				dec->entries[index] = (symbol << 8) | (i + 1);
				index++;
			}
		}
	}

	return dec;
}

bool huff_decode_all(struct huff_dec *dec, struct bit_reader *reader, FILE *out, uint32_t num_sym)
{
	assert(dec    != NULL);
	assert(reader != NULL);
	assert(out    != NULL);

	uint16_t sym;
	if (!bit_reader_next_bits(reader, &sym, dec->max_bits)) {
		fprintf(stderr, "Error while reading input\n");
		return false;
	}

	const uint16_t mask = (1 << dec->max_bits) - 1;

	for (uint32_t i = 0; i < num_sym; i++) {
		uint16_t entry = dec->entries[sym & mask];
		uint8_t symbol = (entry >> 8) & 0xFF;
		uint8_t len    = entry & 0xFF;
		assert(len <= dec->max_bits);

		fwrite(&symbol, 1, 1, out);
		
		if (i == num_sym - 1)
			break;

		uint16_t tmp;
		if (!bit_reader_next_bits(reader, &tmp, len)) {
			fprintf(stderr, "Error while reading input\n");
			return false;
		}
		sym = ((sym << len) | tmp) & mask;
	}

	return true;
}

void huff_destroy(struct huff_dec *dec)
{
	assert(dec != NULL);

	free(dec->entries);
	free(dec);
}

