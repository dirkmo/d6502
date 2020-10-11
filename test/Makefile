all: test.asm
	ca65 test.asm -l test.lst
	ld65 test.o -o test.bin -t none -m test.map -vm

clean:
	rm -f test.lst test.map test.bin test.o
