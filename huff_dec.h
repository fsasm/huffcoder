/*
 * @file huff_dec.h
 * @author Fabjan Sukalia <fsukalia@gmail.com>
 * @date 2016-02-03
 * @brief Huffman tree generation and decoding.
 */

#ifndef HUFF_DEC_H
#define HUFF_DEC_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "bit_reader.h"

struct huff_dec {
	uint8_t max_bits; /* num_entries = 1 << num_bits; */
	uint8_t min_bits;

	/* high byte: symbol; low byte: num_bits; invalid code if num_bits = 0 */
	uint16_t *entries;
};

bool huff_gen_dec(uint8_t code_len[restrict 16], uint8_t symbols[restrict],
				  struct huff_dec * restrict decoder);
bool huff_decode_file(const struct huff_dec * restrict decoder, size_t num_sym,
					  struct bit_reader * restrict reader, FILE *out);
bool huff_decode(const struct huff_dec * restrict decoder, size_t num_sym,
				 struct bit_reader * restrict reader, 
				 uint8_t out_buf[restrict]);
void huff_destroy(struct huff_dec *dec);

#endif

