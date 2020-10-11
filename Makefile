.PHONY: all clean test

CFLAGS=-Wall -g -Wno-unused-function -Wfatal-errors
INC=

SRCS=addressing.c d6502.c instruction_table.c operations.c
OBJS=$(SRCS:.c=.o)

all: sim

lib: d6502.a

%.o: %.c
	gcc $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJS) d6502.a sim.o

test: test/test.asm
	make -C test/

d6502.a: $(OBJS)
	ar -cr $@ $(OBJS)

sim: d6502.a sim.o test
	gcc sim.o d6502.a -o sim
