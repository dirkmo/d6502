#ifndef _CARTRIDGE_H
#define _CARTRIDGE_H

#include <stdint.h>
#include "inesheader.h"

typedef struct {
    inesheader_t header;
    uint8_t *rom_prg16k;
    uint8_t *rom_chr8k;
    uint8_t (*mapper_ppu_read)(uint16_t addr);
    void (*mapper_ppu_write)(uint16_t addr, uint8_t dat);
    uint8_t (*mapper_cpu_read)(uint16_t addr);
    void (*mapper_cpu_write)(uint16_t addr, uint8_t dat);
} cartridge_t;

extern cartridge_t cartridge;

uint8_t cartridge_ppu_read(uint16_t addr);
void cartridge_ppu_write(uint16_t addr, uint8_t dat);

void cartridge_cpu_write(uint16_t addr, uint8_t dat);
uint8_t cartridge_cpu_read(uint16_t addr);

void cartridge_loadROM(const char *fn);
void cartridge_cleanup(void);

#endif
