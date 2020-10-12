#ifndef _D6502_private_h
#define _D6502_private_h

#define PAGE_WRAP(addr1, addr2) (((addr1) >> 8) != ((addr2) >> 8))

typedef enum {
    FLAG_C = 0x01, // carry flag
    FLAG_Z = 0x02, // zero flag
    FLAG_I = 0x04, // int disable
    FLAG_D = 0x08, // decimal mode status flag
    FLAG_B = 0x10, // software int flag (BRK)
    FLAG_R = 0x20, // reserved bit
    FLAG_V = 0x40, // overflow flag
    FLAG_N = 0x80  // sign flag
} flags_t;

uint8_t read(d6502_t *cpu, uint16_t addr);
uint16_t read16(d6502_t *cpu, uint16_t addr);
void write(d6502_t *cpu, uint16_t addr, uint8_t dat);

void set_flag(d6502_t *cpu, uint8_t status_mask, bool flag);
bool get_flag(const d6502_t *cpu, uint8_t status_mask);

#endif
