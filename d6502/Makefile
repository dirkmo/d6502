.PHONY: all clean

CFLAGS=-Wall -g -Wno-unused-function -Wfatal-errors
INC=

SRCS=addressing.c d6502.c instruction_table.c operations.c
OBJS=$(SRCS:.c=.o)

all: d6502.a

%.o: %.c
	gcc $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJS) d6502.a sim.o test.bin

test.bin: test.asm
	ca65 test.asm -l test.lst
	ld65 test.o -o $@ -t none -m $@.map -vm

d6502.a: $(OBJS)
	ar -cr $@ $(OBJS)

sim: d6502.a sim.o test.bin
	gcc sim.o d6502.a -o sim
