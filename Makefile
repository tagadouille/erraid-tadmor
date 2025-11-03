CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2

SRC = $(shell find src -name "*.c")
OBJ = $(SRC:.c=.o)
INC = -I include

EXEC = main

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $(INC) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)

distclean: clean

.PHONY: all clean distclean