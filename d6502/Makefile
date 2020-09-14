.PHONY: all clean

CFLAGS=-Wall -g -Wno-unused-function -Wfatal-errors
INC=

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

all: d6502.a

%.o: %.c
	gcc $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJS) d6502.a

test.bin: test.asm
	ca65 test.asm
	ld65 test.o -o test.bin -t none

d6502.a: $(OBJS)
	ar -cr $@ $(OBJS)