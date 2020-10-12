# d6502
6502 CPU emulation. Used in my NES emulator [DNES](https://github.com/dirkmo/dnes).

## How to use
See comments in the code below.

```c
#include "d6502.h"

uint8_t memory[0x10000]; // 64kB RAM

static uint8_t cpu_read(uint16_t addr) {
    return memory[addr];
}

static void cpu_write(uint16_t addr, uint8_t data) {
    memory[addr] = data;
}

int main(int argc, char *argv[]) {
    // create struct holding the cpu state
    d6502_t cpu;

    // initialize cpu state
    d6502_init( &cpu );

    // set read/write callbacks
    cpu.read = cpu_read;
    cpu.write = cpu_write;

    // trigger reset
    d6502_reset(&cpu);

    while (1) {
        // Every call to d6502_tick() performs a cpu clock cycle.
        // d6502_tick() returns the number of clock cycles left for
        // the current instruction.
        if (d6502_tick(&cpu) == 0) {
            // instruction complete.
            // print next instruction to stdout
            char disassembled_instruction[32];
            d6502_disassemble(&cpu, cpu->pc, disassembled_instruction);
            printf("%s\n", disassembled_instruction);
        }
    }
    return 0;
}
```
