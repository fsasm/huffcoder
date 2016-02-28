# author: Fabjan Sukalia <fsukalia@gmail.com>
# date: 2016-02-03

ifeq ($(CC),cc)
	CC=gcc
endif

ifeq ($(CC),)
	CC=gcc
endif

WARNINGS = -Wall -Wextra -Wundef -Wshadow -Wpointer-arith -Wcast-align \
-Wstrict-prototypes -Wwrite-strings -Waggregate-return
CFLAGS := -std=c99 -pedantic $(WARNINGS) -O2 $(CFLAGS)
LFLAGS := $(LFLAGS)
DEBUG = -g -O0 -fsanitize=address

.PHONY: all clean debug

all: huffdec huffenc

debug: CFLAGS+=$(DEBUG)
debug: all

clean:
	rm -f huffdec huffenc

huffdec: decoder.c bit_reader.c huff_dec.c
	$(CC) $(FLAGS) $(CFLAGS) $(LFLAGS) -o $@ $^

huffenc: encoder.c bit_writer.c huff_enc.c
	$(CC) $(FLAGS) $(CFLAGS) $(LFLAGS) -o $@ $^

