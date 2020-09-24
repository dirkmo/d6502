#ifndef _PPU_H
#define _PPU_H

#include <stdint.h>
#include <stdbool.h>

#define NTABLE0 0x2000u
#define NTABLE1 0x2400u
#define NTABLE2 0x2800u
#define NTABLE3 0x2C00u

#define ATABLE0 (NTABLE0 + 0x03C0)
#define ATABLE1 (NTABLE1 + 0x03C0)
#define ATABLE2 (NTABLE2 + 0x03C0)
#define ATABLE3 (NTABLE3 + 0x03C0)

// total screen incl. sync pixels
#define TOTAL_FRAME_W 341
#define TOTAL_FRAME_H 262
// visible pixels
#define FRAME_W 256
#define FRAME_H 240

// syncs
#define BLANK_W (TOTAL_FRAME_W - FRAME_W)
#define BLANK_H (TOTAL_FRAME_H - FRAME_H)

typedef enum {
    PPUCTRL    = 0x2000,
    PPUMASK    = 0x2001,
    PPUSTATUS  = 0x2002,
    OAMADDR    = 0x2003,
    OAMDATA    = 0x2004,
    PPUSCROLL  = 0x2005,
    PPUADDR    = 0x2006,
    PPUDATA    = 0x2007,
    OAMDMA 	   = 0x4014,
} ppu_register;

bool ppu_interrupt(void);
const uint32_t *ppu_getFrameBuffer(void);
bool ppu_should_draw(void);

void ppu_write(uint8_t addr, uint8_t dat);
uint8_t ppu_read(uint8_t addr);

void ppu_tick(void);

#endif