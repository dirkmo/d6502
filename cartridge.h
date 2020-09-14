#ifndef _CARTRIDGE_H
#define _CARTRIDGE_H

#include <stdint.h>
#include "inesheader.h"

typedef struct {
    inesheader_t header;
    uint8_t *rom_prg16k;
    uint8_t *rom_chr8k;
} cartridge_t;


const uint8_t *cartridge_getCHR8k(uint8_t chr_idx);

void cartridge_write(uint16_t addr, uint8_t dat);
uint8_t cartridge_read(uint16_t addr);

void cartridge_loadROM(const char *fn);
void cartridge_cleanup(void);

#endif
