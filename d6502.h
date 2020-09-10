#ifndef __D6502
#define __D6502

#include <stdint.h>
#include <stdbool.h>

#define ENABLE_DECIMAL_MODE false

struct d6502_s;
typedef struct d6502_s d6502_t;

typedef struct {
    uint8_t opcode;
    const char *mnemonic;
    uint8_t len;
    uint8_t cycles;
    void (*operation)(d6502_t *cpu);
    void (*addressing)(d6502_t *cpu);
} instruction_t;

struct d6502_s {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t st;
    uint16_t pc;
    uint8_t sp;

    bool interrupt;
    bool nmi;
    
    uint16_t addr; // address to read/write, set in addressing mode function
    uint8_t m; // temporary register, set in addressing mode function
    const instruction_t *instruction;
    uint8_t extra_clocks;
    uint8_t current_cycle; // counts ticks for current instruction
    char disassemble[16];

    void (*write)(uint16_t addr, uint8_t dat);
    uint8_t (*read)(uint16_t addr);
};

extern int EMULATION_END;

void d6502_init(d6502_t *cpu);
int d6502_tick(d6502_t *cpu);
void d6502_disassemble(d6502_t *cpu, uint16_t addr, char *asmcode);
void d6502_reset(d6502_t *cpu);
void d6502_interrupt(d6502_t *cpu);
void d6502_nmi(d6502_t *cpu);

#endif