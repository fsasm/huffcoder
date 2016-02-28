/*
 * @file huff_enc.h
 * @author Fabjan Sukalia <fsukalia@gmail.com>
 * @date 2016-02-16
 * @brief Huffman code (canonical) generation and encoding
 */

#ifndef HUFF_ENC_H
#define HUFF_ENC_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "bit_writer.h"

struct huff_code {
	uint16_t code;
	uint8_t  code_len;
	uint8_t  symbol;
};

struct huff_enc {
	struct huff_code *codes;
	uint16_t  num_codes;
};

struct huff_enc_info {
	uint16_t num_codes;
	uint8_t  min_bits;
	uint8_t  max_bits;
	uint8_t  codes_per_len[16];
};

void huff_get_freq(const uint8_t data[restrict], size_t size, 
				   uint32_t freq[restrict 256]);
bool huff_gen_enc(const uint32_t freq[restrict 256],
				  struct huff_enc * restrict encoder, 
				  struct huff_enc_info * restrict info);
void huff_enc_destroy(struct huff_enc *encoder);
bool huff_encode(const struct huff_enc * restrict encoder, size_t num_sym,
				 const uint8_t in_data[restrict], 
				 struct bit_writer * restrict writer);

#endif

