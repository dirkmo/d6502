#ifndef _PPU_H
#define _PPU_H

#include <stdint.h>

int ppu_init_sdl(void);

void ppu_write(uint8_t addr, uint8_t dat);
uint8_t ppu_read(uint8_t addr);

void ppu_tick(void);

#endif
