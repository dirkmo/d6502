#ifndef __ADDRESSING_H
#define __ADDRESSING_H 

#include "d6502.h"

void Implied(d6502_t *cpu);
void Accumulator(d6502_t *cpu);
void Immediate(d6502_t *cpu);
void Indirect(d6502_t *cpu);
void IndirectX(d6502_t *cpu);
void IndirectY(d6502_t *cpu);
void ZeroPage(d6502_t *cpu);
void ZeroPageX(d6502_t *cpu);
void ZeroPageY(d6502_t *cpu);
void AbsoluteX(d6502_t *cpu);
void AbsoluteY(d6502_t *cpu);
void Absolute(d6502_t *cpu);
void Relative(d6502_t *cpu);

#endif
