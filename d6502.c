#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "d6502.h"
#include "d6502_private.h"
#include "operations.h"
#include "instruction_table.h"

void set_flag(d6502_t *cpu, uint8_t status_mask, bool flag) {
    cpu->st = flag ? cpu->st | status_mask : cpu->st & ~status_mask;
}

bool get_flag(const d6502_t *cpu, uint8_t status_mask) {
    return (cpu->st & status_mask) != 0;
}

uint16_t read16(d6502_t *cpu, uint16_t addr) {
    return ((uint16_t)cpu->read(addr)) | ((uint16_t)cpu->read(addr+1) << 8);
}

static void fetch(d6502_t *cpu) {
    uint8_t opcode;
    if (cpu->nmi || cpu->interrupt) {
        opcode = 0x00; // BRK opcode
    } else {
        opcode = cpu->read(cpu->pc);
    }
    cpu->instruction = get_instruction(opcode);
}

// execute:
// call addressing_mode(), execute instruction, move PC to next instruction
static void execute(d6502_t *cpu) {
    cpu->extra_clocks = 0;
    if (cpu->instruction->addressing == NULL) {
        // illegal instruction -> perform NOP
        cpu->instruction = get_instruction(0xEA);
    }
    cpu->instruction->addressing(cpu); // sets cpu->addr
    if(!cpu->nmi && !cpu->interrupt) { // might skip output on race condition
        //printf("%s %s\n", cpu->instruction->mnemonic, cpu->disassemble);
    } else {
        // printf("%s\n", cpu->nmi ? "NMI" : "interrupt");
    }
    
    cpu->instruction->operation(cpu);
    cpu->pc += cpu->instruction->len;
    cpu->current_cycle = cpu->instruction->cycles + cpu->extra_clocks;
}

void d6502_init(d6502_t *cpu) {
    initialize_instructions_sorted();
    cpu->instruction = get_instruction(0xEA); // NOP
    cpu->extra_clocks = 0;
    cpu->current_cycle = 0;
    cpu->nmi = false;
    cpu->interrupt = false;
}

int d6502_tick(d6502_t *cpu) {
    if(cpu->current_cycle == 0) {
        fetch(cpu);
        execute(cpu);
        if(cpu->nmi) {
            cpu->nmi = false;
        } else if(cpu->interrupt) {
            cpu->interrupt = false;
        }
    } else {
        cpu->current_cycle--;
    }
    return cpu->current_cycle;
}

void d6502_disassemble(d6502_t *cpu, uint16_t addr, char *asmcode) {
    d6502_t tempcpu = *cpu;
    tempcpu.pc = addr;
    uint8_t opcode = cpu->read(tempcpu.pc);
    tempcpu.instruction = get_instruction(opcode);
    if( tempcpu.instruction->addressing) {
        tempcpu.instruction->addressing(&tempcpu);
        strcpy(asmcode, tempcpu.instruction->mnemonic);
    } else {
        // undefined opcode -> perform NOP
        tempcpu.instruction = get_instruction(0xEA);
        strcpy(asmcode, "INVALD");
    }
    strcat(strcat(asmcode, " "), tempcpu.disassemble);
}

void d6502_reset(d6502_t *cpu) {
    cpu->pc = read16(cpu, RESET_ADDR);
    cpu->st = FLAG_I | FLAG_R;
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    cpu->sp = 0xfd;
}

void d6502_interrupt(d6502_t *cpu) {
    cpu->interrupt = !get_flag(cpu, FLAG_I);
}

void d6502_nmi(d6502_t *cpu) {
    cpu->nmi = true;
}
