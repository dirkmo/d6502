#ifndef __OPERATIONS_H
#define __OPERATIONS_H

void ADC(d6502_t *cpu);
void AND(d6502_t *cpu);
void ASL(d6502_t *cpu);
void BCC(d6502_t *cpu);
void BCS(d6502_t *cpu);
void BEQ(d6502_t *cpu);
void BIT(d6502_t *cpu);
void BMI(d6502_t *cpu);
void BNE(d6502_t *cpu);
void BPL(d6502_t *cpu);
void BRK(d6502_t *cpu);
void BVC(d6502_t *cpu);
void BVS(d6502_t *cpu);
void CLC(d6502_t *cpu);
void CLD(d6502_t *cpu);
void CLI(d6502_t *cpu);
void CLV(d6502_t *cpu);
void CMP(d6502_t *cpu);
void CPX(d6502_t *cpu);
void CPY(d6502_t *cpu);
void DEC(d6502_t *cpu);
void DEX(d6502_t *cpu);
void DEY(d6502_t *cpu);
void EOR(d6502_t *cpu);
void INC(d6502_t *cpu);
void INX(d6502_t *cpu);
void INY(d6502_t *cpu);
void JMP(d6502_t *cpu);
void JSR(d6502_t *cpu);
void LDA(d6502_t *cpu);
void LDX(d6502_t *cpu);
void LDY(d6502_t *cpu);
void LSR(d6502_t *cpu);
void NOP(d6502_t *cpu);
void ORA(d6502_t *cpu);
void PHA(d6502_t *cpu);
void PHP(d6502_t *cpu);
void PLA(d6502_t *cpu);
void PLP(d6502_t *cpu);
void ROL(d6502_t *cpu);
void ROR(d6502_t *cpu);
void RTI(d6502_t *cpu);
void RTS(d6502_t *cpu);
void SBC(d6502_t *cpu);
void SEC(d6502_t *cpu);
void SED(d6502_t *cpu);
void SEI(d6502_t *cpu);
void STA(d6502_t *cpu);
void STX(d6502_t *cpu);
void STY(d6502_t *cpu);
void TAX(d6502_t *cpu);
void TAY(d6502_t *cpu);
void TSX(d6502_t *cpu);
void TXA(d6502_t *cpu);
void TXS(d6502_t *cpu);
void TYA(d6502_t *cpu);

// illegal instructions
void LAX(d6502_t *cpu);
void SAX(d6502_t *cpu);
void iSBC(d6502_t *cpu);
void DCP(d6502_t *cpu);
void ILL(d6502_t *cpu);

void END(d6502_t *cpu);


#endif
