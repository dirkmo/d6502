#ifndef _INSTRUCTION_TABLE_H
#define _INSTRUCTION_TABLE_H

#include "d6502.h"

void initialize_instructions_sorted(void);

const instruction_t *get_instruction(uint8_t opcode);

#endif
