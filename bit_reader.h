/*
 * @file bit_reader.h
 * @author Fabjan Sukalia <fsukalia@gmail.com>
 * @date 2016-02-03
 * @brief Module to read bitwise from a file.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

struct bit_reader;

struct bit_reader *bit_reader_create(FILE *in);
void bit_reader_destroy(struct bit_reader *reader);
bool bit_reader_next_bit(struct bit_reader *reader, uint8_t *bit);
bool bit_reader_next_bits(struct bit_reader *reader, uint16_t *bits, uint8_t num);

