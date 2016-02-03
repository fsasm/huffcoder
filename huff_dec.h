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

struct huff_dec;

struct huff_dec *huff_gen_dec(uint8_t code_len[restrict 16], uint8_t symbols[restrict]);
bool huff_decode_all(struct huff_dec *tree, struct bit_reader *reader, FILE *out, uint32_t num_sym);
void huff_destroy(struct huff_dec *tree);

#endif

