.PHONY: all clean

CFLAGS=-Wall -g -Wno-unused-function -Wfatal-errors
INC=
LDFLAGS=

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

BIN=d6502

all: $(OBJS) test.bin
	gcc $(OBJS) $(LDFLAGS) -o $(BIN)

%.o: %.c
	gcc $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJS) $(BIN) test.bin

test.bin: test.asm
	ca65 test.asm
	ld65 test.o -o test.bin -t none
