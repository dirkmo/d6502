#include "d6502.h"
#include "d6502_private.h"
#include "addressing.h"
#include <stdio.h>

// Endianess: low byte on lower address

#define IS_ACC_ADDRESSING(cpu) (cpu->instruction->addressing == Accumulator)

static uint8_t read_addr(d6502_t * cpu) {
    return cpu->read(cpu->addr);
}

static void push8(d6502_t *cpu, uint8_t dat) {
    cpu->write(0x100 + cpu->sp, dat);
    cpu->sp--;
}

static void push16(d6502_t *cpu, uint16_t dat) {
    push8(cpu, dat >> 8);
    push8(cpu, dat);
}

static uint8_t pull8(d6502_t *cpu) {
    cpu->sp++;
    return cpu->read(0x100 + cpu->sp);
}

static uint16_t pull16(d6502_t *cpu) {
    uint16_t dat = pull8(cpu);
    return dat | ((uint16_t)pull8(cpu) << 8);
}

static void branch_on_condition(d6502_t *cpu, bool condition) {
    if (condition) {
        if( (cpu->pc >> 8) != (cpu->addr >> 8)) {
            cpu->extra_clocks = 1;
        }
        cpu->pc = cpu->addr;
    }
}

void ADC(d6502_t *cpu) { // add with carry
    uint8_t src = read_addr(cpu);
    unsigned int temp = src + cpu->a + (get_flag(cpu, FLAG_C) ? 1 : 0);
    set_flag(cpu, FLAG_Z, (temp & 0xff) == 0);	/* This is not valid in decimal mode */
    if (ENABLE_DECIMAL_MODE && get_flag(cpu, FLAG_D)) {
        if (((cpu->a & 0xf) + (src & 0xf) + (get_flag(cpu, FLAG_C) ? 1 : 0)) > 9) {
            temp += 6;
        }
        set_flag(cpu, FLAG_N, temp & 0x80);
        set_flag(cpu, FLAG_V, !((cpu->a ^ src) & 0x80) && ((cpu->a ^ temp) & 0x80));
    	if (temp > 0x99) {
            temp += 96;
        }
        set_flag(cpu, FLAG_C, temp > 0x99);
    } else {
        set_flag(cpu, FLAG_N, temp & 0x80);
        set_flag(cpu, FLAG_V, !((cpu->a ^ src) & 0x80) && ((cpu->a ^ temp) & 0x80));
        set_flag(cpu, FLAG_C, temp > 0xff);
    }
    cpu->a = (uint8_t) temp;
}

void AND(d6502_t *cpu) { // logical and
    uint8_t op = read_addr(cpu);
    cpu->a = op & cpu->a;
    set_flag(cpu, FLAG_Z, cpu->a == 0);
    set_flag(cpu, FLAG_N, (cpu->a & 0x80) != 0);
}

void ASL(d6502_t *cpu) { // arithmetic shift left
    uint8_t src = (IS_ACC_ADDRESSING(cpu)) ? cpu->a : read_addr(cpu);
    set_flag(cpu, FLAG_C, src & 0x80);
    src = src << 1;
    set_flag(cpu, FLAG_Z, src == 0);
    set_flag(cpu, FLAG_N, (src & 0x80) != 0);
    if( IS_ACC_ADDRESSING(cpu) ) {
        cpu->a = src;
    } else {
        cpu->write(cpu->addr, src);
    }
}

void BCC(d6502_t *cpu) { // branch if carry is clear
    branch_on_condition(cpu, !get_flag(cpu, FLAG_C));
}

void BCS(d6502_t *cpu) { // branch if carry is set
    branch_on_condition(cpu, get_flag(cpu, FLAG_C));
}

void BEQ(d6502_t *cpu) { // branch if zero
    branch_on_condition(cpu, get_flag(cpu, FLAG_Z));
}

void BIT(d6502_t *cpu) { // Test bits in memory with accumulator
    uint8_t src = read_addr(cpu);
    set_flag(cpu, FLAG_N, (src & 0x80) != 0);
    set_flag(cpu, FLAG_V, (0x40 & src) != 0);
    set_flag(cpu, FLAG_Z, (src & cpu->a) == 0);
}

void BMI(d6502_t *cpu) { // Branch on result minus
    branch_on_condition(cpu, get_flag(cpu, FLAG_N));
}

void BNE(d6502_t *cpu) { // Branch on result not zero
    branch_on_condition(cpu, !get_flag(cpu, FLAG_Z));
}

void BPL(d6502_t *cpu) { // Branch on result plus
    branch_on_condition(cpu, !get_flag(cpu, FLAG_N));
}

void BRK(d6502_t *cpu) { // Force Break and interrupt/nmi
    bool intr = cpu->nmi || cpu->interrupt;
    uint16_t vector = INT_ADDR;
    uint16_t pc = cpu->pc + intr ? 0 : 2;
    push16(cpu, pc);
    // http://visual6502.org/wiki/index.php?title=6502_BRK_and_B_bit
    // software instructions BRK & PHP will push the B flag as being 1
    // hardware interrupts IRQ & NMI will push the B flag as being 0
    uint8_t st = cpu->st | intr ? 0 : FLAG_B;
    push8(cpu, st);
    set_flag(cpu, FLAG_I, 1);
    if (cpu->nmi) {
        vector = NMI_ADDR;
    }
    cpu->pc = read16(cpu, vector) - cpu->instruction->len;
}

void BVC(d6502_t *cpu) { // Branch on overflow clear
    branch_on_condition(cpu, !get_flag(cpu, FLAG_V));
}

void BVS(d6502_t *cpu) { // Branch on overflow set
    branch_on_condition(cpu, get_flag(cpu, FLAG_V)); // Branch on overflow set
}

void CLC(d6502_t *cpu) { // Clear carry flag
    set_flag(cpu, FLAG_C, 0);
}

void CLD(d6502_t *cpu) { // Clear decimal mode
    set_flag(cpu, FLAG_D, 0);
}

void CLI(d6502_t *cpu) { // Clear interrupt disable bit
    set_flag(cpu, FLAG_I, 0);
}

void CLV(d6502_t *cpu) { // Clear overflow flag
    set_flag(cpu, FLAG_V, 0);
}

void CMP(d6502_t *cpu) { // Compare memory and accumulator
    uint16_t m = read_addr(cpu);
    uint16_t a = cpu->a;
    m = a - m;
    set_flag(cpu, FLAG_C, m < 0x100);
    set_flag(cpu, FLAG_N, (m & 0x80) > 0);
    set_flag(cpu, FLAG_Z, m == 0);
}

void CPX(d6502_t *cpu) { // Compare Memory and Index X
    uint16_t m = read_addr(cpu);
    uint16_t x = cpu->x;
    m = x - m;
    set_flag(cpu, FLAG_C, m < 0x100);
    set_flag(cpu, FLAG_N, (m & 0x80) > 0);
    set_flag(cpu, FLAG_Z, m == 0);
}

void CPY(d6502_t *cpu) { // Compare memory and index Y
    uint16_t m = read_addr(cpu);
    uint16_t y = cpu->y;
    m = y - m;
    set_flag(cpu, FLAG_C, m < 0x100);
    set_flag(cpu, FLAG_N, (m & 0x80) > 0);
    set_flag(cpu, FLAG_Z, m == 0);
}

void DEC(d6502_t *cpu) { // Decrement memory by one
    uint8_t m = read_addr(cpu) - 1;
    set_flag(cpu, FLAG_N, (m & 0x80) > 0);
    set_flag(cpu, FLAG_Z, m == 0);
    cpu->write(cpu->addr, m);
}

void DEX(d6502_t *cpu) { // Decrement index X by one
    cpu->x--;
    set_flag(cpu, FLAG_N, (cpu->x & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->x == 0);
}

void DEY(d6502_t *cpu) { // Decrement index Y by one
    cpu->y--;
    set_flag(cpu, FLAG_N, (cpu->y & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->y == 0);
}

void EOR(d6502_t *cpu) { // "Exclusive-Or" memory with accumulator
    uint8_t m = read_addr(cpu);
    cpu->a ^= m;
    set_flag(cpu, FLAG_N, (cpu->a & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->a == 0);
}

void INC(d6502_t *cpu) { // Increment memory by one
    uint8_t m = read_addr(cpu) + 1;
    set_flag(cpu, FLAG_N, (m & 0x80) > 0);
    set_flag(cpu, FLAG_Z, m == 0);
    cpu->write(cpu->addr, m);
}

void INX(d6502_t *cpu) { // Increment Index X by one
    cpu->x++;
    set_flag(cpu, FLAG_N, (cpu->x & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->x == 0);
}

void INY(d6502_t *cpu) { // Increment Index Y by one
    cpu->y++;
    set_flag(cpu, FLAG_N, (cpu->y & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->y == 0);
}

void JMP(d6502_t *cpu) { // Jump to new location
    cpu->pc = cpu->addr - cpu->instruction->len;
}

void JSR(d6502_t *cpu) { // Jump to new location saving return address
    push16(cpu, cpu->pc + 2);
    cpu->pc = cpu->addr - cpu->instruction->len;
}

void LDA(d6502_t *cpu) { // Load accumulator with memory
    cpu->a = read_addr(cpu);
    set_flag(cpu, FLAG_N, (cpu->a & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->a == 0);
}

void LDX(d6502_t *cpu) { // Load index X with memory
    cpu->x = read_addr(cpu);
    set_flag(cpu, FLAG_N, (cpu->x & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->x == 0);
}

void LDY(d6502_t *cpu) { // Load index Y with memory
    cpu->y = read_addr(cpu);
    set_flag(cpu, FLAG_N, (cpu->y & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->y == 0);
}

void LSR(d6502_t *cpu) { // Shift right one bit (memory or accumulator)
    uint8_t m = IS_ACC_ADDRESSING(cpu) ? cpu->a : read_addr(cpu);
    set_flag(cpu, FLAG_C, m & 1);
    m = m >> 1;
    if (IS_ACC_ADDRESSING(cpu)) {
        cpu->a = m;
    } else {
        cpu->write(cpu->addr, m);
    }
    set_flag(cpu, FLAG_Z, m == 0);
    set_flag(cpu, FLAG_N, 0);
}

void NOP(d6502_t *cpu) { // No operation
}

void ORA(d6502_t *cpu) { // "OR" memory with accumulator
    uint8_t m = read_addr(cpu);
    cpu->a = cpu->a | m;
    set_flag(cpu, FLAG_N, (cpu->a & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->a == 0);
}

void PHA(d6502_t *cpu) { // Push accumulator on stack 
    push8(cpu, cpu->a);
}

void PHP(d6502_t *cpu) { // Push processor status on stack
    push8(cpu, cpu->st | FLAG_B);
}

void PLA(d6502_t *cpu) { // Pull accumulator from stack
    cpu->a = pull8(cpu);
    set_flag(cpu, FLAG_N, (cpu->a & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->a == 0);
}

void PLP(d6502_t *cpu) { // Pull processor status from stack
    uint8_t status = pull8(cpu) & ~FLAG_B;
    status |= FLAG_R; // FLAG_R is always set
    cpu->st = status;
}

void ROL(d6502_t *cpu) { // Rotate one bit left (memory or accumulator)
    uint16_t m = IS_ACC_ADDRESSING(cpu) ? cpu->a : read_addr(cpu);
    m = (m << 1) | (get_flag(cpu, FLAG_C) ? 1 : 0);
    set_flag(cpu, FLAG_C, m > 0xff );
    m &= 0xFF;
    set_flag(cpu, FLAG_N, (m & 0x80) != 0);
    set_flag(cpu, FLAG_Z, m == 0);
    if (IS_ACC_ADDRESSING(cpu)) {
        cpu->a = (uint8_t)m;
    } else {
        cpu->write(cpu->addr, m);
    }
}

void ROR(d6502_t *cpu) { // Rotate one bit right (memory or accumulator)
    uint16_t m = IS_ACC_ADDRESSING(cpu) ? cpu->a : read_addr(cpu);
    m |= get_flag(cpu, FLAG_C) ? 0x100 : 0;
    set_flag(cpu, FLAG_C, m & 1); // put old bit 0 to carry flag
    m = (m >> 1);
    set_flag(cpu, FLAG_Z, m == 0);
    set_flag(cpu, FLAG_N, (m & 0x80) != 0);
    if (IS_ACC_ADDRESSING(cpu)) {
        cpu->a = (uint8_t)m;
    } else {
        cpu->write(cpu->addr, m);
    }
}

void RTI(d6502_t *cpu) { // RTI Return from interrupt
    cpu->st = pull8(cpu) | FLAG_R;
    cpu->pc = pull16(cpu) - cpu->instruction->len;
}

void RTS(d6502_t *cpu) { // Return from subroutine
    cpu->pc = pull16(cpu) + 1;
    cpu->pc -= cpu->instruction->len;
}

void SBC(d6502_t *cpu) { // Subtract memory from accumulator with borrow
    uint8_t m = read_addr(cpu);
    unsigned int temp = cpu->a - m - (get_flag(cpu, FLAG_C) ? 0 : 1);
    set_flag(cpu, FLAG_N, temp & 0x80);
    set_flag(cpu, FLAG_Z, (temp & 0xff) == 0);	/* Sign and Zero are invalid in decimal mode */
    set_flag(cpu, FLAG_V, ((cpu->a ^ temp) & 0x80) && ((cpu->a ^ m) & 0x80));
    if (ENABLE_DECIMAL_MODE && get_flag(cpu, FLAG_D)) {
        if ( ((cpu->a & 0xf) - (get_flag(cpu, FLAG_C) ? 0 : 1)) < (m & 0xf)) /* EP */
            temp -= 6;
        if (temp > 0x99)
            temp -= 0x60;
    }
    set_flag(cpu, FLAG_C, temp < 0x100);
    cpu->a = (temp & 0xff);
}

void SEC(d6502_t *cpu) { // Set carry flag
    set_flag(cpu, FLAG_C, 1);
}

void SED(d6502_t *cpu) { // Set decimal mode
    set_flag(cpu, FLAG_D, 1);
}

void SEI(d6502_t *cpu) { // Set interrupt disable status
    set_flag(cpu, FLAG_I, 1);
}

void STA(d6502_t *cpu) { // Store accumulator in memory
    cpu->write(cpu->addr, cpu->a);
}

void STX(d6502_t *cpu) { // Store index X in memory
    cpu->write(cpu->addr, cpu->x);
}

void STY(d6502_t *cpu) { // Store index Y in memory
    cpu->write(cpu->addr, cpu->y);
}

void TAX(d6502_t *cpu) { // Transfer accumulator to index X
    cpu->x = cpu->a;
    set_flag(cpu, FLAG_N, (cpu->x & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->x == 0);
}

void TAY(d6502_t *cpu) { // Transfer accumulator to index Y
    cpu->y = cpu->a;
    set_flag(cpu, FLAG_N, (cpu->y & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->y == 0);
}

void TSX(d6502_t *cpu) { // Transfer stack pointer to index X
    cpu->x = cpu->sp;
    set_flag(cpu, FLAG_N, (cpu->x & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->x == 0);
}

void TXA(d6502_t *cpu) { // Transfer index X to accumulator
    cpu->a = cpu->x;
    set_flag(cpu, FLAG_N, (cpu->a & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->a == 0);
}

void TXS(d6502_t *cpu) { // Transfer index X to stack pointer
    cpu->sp = cpu->x;
}

void TYA(d6502_t *cpu) { // Transfer index Y to accumulator
    cpu->a = cpu->y;
    set_flag(cpu, FLAG_N, (cpu->a & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->a == 0);
}

void LAX(d6502_t *cpu) { // illegal: LDA then TAX
    cpu->a = read_addr(cpu);
    cpu->x = cpu->a;
    set_flag(cpu, FLAG_N, (cpu->a & 0x80) > 0);
    set_flag(cpu, FLAG_Z, cpu->a == 0);
}

void SAX(d6502_t *cpu) { // illegal: mem = (A & X)
    cpu->write(cpu->addr, cpu->a & cpu->x);
}

void iSBC(d6502_t *cpu) {
    uint8_t m = read_addr(cpu);
    unsigned int temp = cpu->a - m - (get_flag(cpu, FLAG_C) ? 0 : 1);
    set_flag(cpu, FLAG_N, temp & 0x80);
    set_flag(cpu, FLAG_Z, (temp & 0xff) == 0);	/* Sign and Zero are invalid in decimal mode */
    set_flag(cpu, FLAG_V, ((cpu->a ^ temp) & 0x80) && ((cpu->a ^ m) & 0x80));
    set_flag(cpu, FLAG_C, temp < 0x100);
    cpu->a = (temp & 0xff);
}

void DCP(d6502_t *cpu) {
    // TODO not working properly
    uint8_t m = read_addr(cpu);
    cpu->write(cpu->addr, m-1);
    uint16_t a = cpu->a;
    m = a - m;
    set_flag(cpu, FLAG_N, (m & 0x80) > 0);
    set_flag(cpu, FLAG_Z, m == 0);
}

void ILL(d6502_t *cpu) { // illegal
}

void END(d6502_t *cpu) { // end emulator
    EMULATION_END = 1;
}