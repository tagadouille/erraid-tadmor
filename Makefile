CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2
INC = -I include

SRC_MAIN = src/main.c src/erraid.c
SRC_TEST = src/test_reader.c
SRC_COMMON = $(shell find src -path src/main.c -prune -o -name "*.c" -print)

OBJ_MAIN = $(SRC_MAIN:.c=.o) $(SRC_COMMON:.c=.o)
OBJ_TEST = $(SRC_TEST:.c=.o) $(SRC_COMMON:.c=.o)

all: erraid test_reader

erraid: $(OBJ_MAIN)
	$(CC) $(CFLAGS) $(INC) -o $@ $^

test_reader: $(OBJ_TEST)
	$(CC) $(CFLAGS) $(INC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

distclean:
	rm -f $(OBJ_MAIN) $(OBJ_TEST) erraid test_reader

.PHONY: all clean
