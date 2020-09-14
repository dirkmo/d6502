.PHONY: all clean

CFLAGS=-Wall -g -Wno-unused-function -Wfatal-errors
INC=-Id6502
LDFLAGS=-lSDL2

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

BIN=dnes

all: $(OBJS) d6502.a
	gcc $(OBJS) d6502/d6502.a $(LDFLAGS) -o $(BIN)

%.o: %.c
	gcc $(CFLAGS) $(INC) -c $< -o $@

clean:
	make -C d6502/ clean
	rm -f $(OBJS) $(BIN)

d6502.a:
	make -C d6502