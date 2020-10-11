#include "addressing.h"
#include "d6502_private.h"
#include <stdio.h>

static uint8_t immediate8(d6502_t *cpu) {
    return cpu->read(cpu->pc + 1);
}

static uint16_t immediate16(d6502_t *cpu) {
    return read16(cpu, cpu->pc + 1);
}

void Implied(d6502_t *cpu) {
    cpu->disassemble[0] = 0;
}

void Accumulator(d6502_t *cpu) {
    sprintf(cpu->disassemble, "A");
}

void Immediate(d6502_t *cpu) {
    // LDA #$0A
    cpu->addr = cpu->pc + 1;
    sprintf(cpu->disassemble, "#$%02X", immediate8(cpu));
}

void Indirect(d6502_t *cpu) {
    // JMP ($1234)
    uint16_t imm = immediate16(cpu);
    uint16_t hi = imm & 0xff00;
    uint8_t lo = imm & 0xff;
    cpu->addr = cpu->read(hi | lo++);
    cpu->addr |= cpu->read(hi | lo) << 8;
    sprintf(cpu->disassemble, "($%04X)", imm);
}

void IndirectX(d6502_t *cpu) {
    // LDA ($3E, X)
    uint8_t zp_addr = immediate8(cpu);
    uint8_t addr = zp_addr + cpu->x; // using uint8 for zeropage-wrap-around
    cpu->addr = cpu->read(addr++);
    cpu->addr |= cpu->read(addr) << 8;
    sprintf(cpu->disassemble, "($%02X,X)", zp_addr);
}

void IndirectY(d6502_t *cpu) {
    // LDA ($4C), Y
    uint16_t zp_addr = immediate8(cpu);
    uint8_t addr = zp_addr; // using uint8 for zeropage-wrap-around
    cpu->addr = cpu->read(addr++);
    cpu->addr |= ((uint16_t)cpu->read(addr)) << 8;
    if (PAGE_WRAP(cpu->addr, cpu->addr + cpu->y)) {
        cpu->extra_clocks++;
    }
    cpu->addr += cpu->y;
    sprintf(cpu->disassemble, "($%02X),Y", zp_addr);
}

void ZeroPage(d6502_t *cpu) {
    // LDA $20
    cpu->addr = immediate8(cpu);
    sprintf(cpu->disassemble, "$%02X", cpu->addr);
}

void ZeroPageX(d6502_t *cpu) {
    // LDA $20, X
    uint8_t zp_addr = immediate8(cpu);
    cpu->addr = (zp_addr + cpu->x) & 0xFF;
    sprintf(cpu->disassemble, "$%02X,X", zp_addr);
}

void ZeroPageY(d6502_t *cpu) {
    // LDX $10, Y
    uint8_t zp_addr = immediate8(cpu);
    cpu->addr = (zp_addr + cpu->y) & 0xFF;
    sprintf(cpu->disassemble, "$%02X,Y", zp_addr);
}

void AbsoluteX(d6502_t *cpu) {
    // LDA $1234, X
    uint16_t a1 = immediate16(cpu);
    cpu->addr = a1 + cpu->x;
    sprintf(cpu->disassemble, "$%04X,X", a1);
}

void AbsoluteY(d6502_t *cpu) {
    // LDA $1234, Y
    uint16_t a1 = immediate16(cpu);
    cpu->addr = a1 + cpu->y;
    sprintf(cpu->disassemble, "$%04X,Y", a1);
}

void Absolute(d6502_t *cpu) {
    cpu->addr = immediate16(cpu);
    sprintf(cpu->disassemble, "$%04X", cpu->addr);
}

void Relative(d6502_t *cpu) {
    uint16_t im = immediate8(cpu);
    if (im & 0x80) {
        im |= 0xFF00;
    }
    cpu->addr = cpu->pc + im;
    uint16_t disp = cpu->addr + cpu->instruction->len;
    sprintf(cpu->disassemble, "$%02X", disp);
}
