/*
 * @file bit_writer.h
 * @author Fabjan Sukalia <fsukalia@gmail.com>
 * @date 2016-02-16
 * @brief Module to write bitwise to a file.
 */

#ifndef BIT_WRITER_H
#define BIT_WRITER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

struct bit_writer;

struct bit_writer *bit_writer_create(FILE *out);
void bit_writer_destroy(struct bit_writer *writer);

bool bit_writer_next_bit(struct bit_writer *writer, uint8_t bit);
bool bit_writer_next_bits(struct bit_writer *writer, uint16_t bits, uint8_t num);

#endif

