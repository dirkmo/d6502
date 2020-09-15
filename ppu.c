#include "ppu.h"

#include <assert.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "palette.h"
#include "cartridge.h"

#define W 320
#define H 200

static SDL_Window *win = NULL;
static SDL_Renderer *ren = NULL;
static SDL_Texture *tex = NULL;

static uint8_t pixels[W*H*3] = { 0 };

uint8_t ppu_ctrl1 = 0;
uint8_t ppu_ctrl2 = 0;
uint8_t ppu_status = 0;
uint8_t ppu_spraddr = 0;
uint8_t ppu_sprio = 0;
uint16_t ppuaddr = 0;
uint8_t ppudata = 0;

uint8_t vram[0x4000] = { 0 };

static uint32_t tick = 0;


void setpixel( int x, int y, uint8_t  r, uint8_t g, uint8_t b ) {
    int p = (y * W + x) * 3;
    if( p+2 >= sizeof(pixels)) {
        printf("x: %d, y: %d\n", x, y);
        assert(p+2 < sizeof(pixels));
    }
    pixels[p++] = b;
    pixels[p++] = g;
    pixels[p] = r;
}

uint8_t getTilePixel(const uint8_t *rawtile, uint8_t idx) {
    int bitidx = idx % 8;
    int byteidx = (idx / 8);
    uint8_t b1 = (rawtile[byteidx] >> (7-bitidx)) & 1;
    uint8_t b2 = (rawtile[byteidx+8] >> (7-bitidx)) & 1;
    return b1 | (b2 << 1);
}

void drawTile( const uint8_t *rawtile, const uint8_t *colors, int x, int y) {
    for ( int i = 0; i < 64; i++ ) {
        uint8_t pix = colors[getTilePixel(rawtile, i)] * 3;
        int b = palette[pix];
        int g = palette[pix+1];
        int r = palette[pix+2];
        setpixel(x+i%8, y+i/8, r, g, b);
    }
}

const uint8_t *getTile(uint8_t chr_idx, uint8_t idx) {
    return cartridge_getCHR8k(chr_idx) + 16 * idx;
}

void draw(void) {
    uint8_t colors[] = { 0xf, 0x27, 0x2a, 0x2b};
    const uint8_t *tile;
    for( int i = 0; i < 256; i++ ) {
        tile = getTile(0, i);
        drawTile(tile, colors, (i%16)*8, (i/16)*8);
    }

    SDL_UpdateTexture(tex, NULL, pixels, W*3);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
 }

uint8_t getAttribute(uint8_t *table, uint8_t x, uint8_t y) {
    // 256x240, 8 Attribute bytes per line

    // +------------+------------+
    // |  Square 0  |  Square 1  |   A square is 16x16 pixels = 4 tiles
    // |   #0  #1   |   #4  #5   |
    // |   #2  #3   |   #6  #7   |
    // +------------+------------+
    // |  Square 2  |  Square 3  |
    // |   #8  #9   |   #C  #D   |
    // |   #A  #B   |   #E  #F   |
    // +------------+------------+

    //   Attribute Byte
    //    (Square #)
    //  ----------------
    //      33221100
    //      ||||||+--- Upper two (2) colour bits for Square 0 (Tiles #0,1,2,3)
    //      ||||+----- Upper two (2) colour bits for Square 1 (Tiles #4,5,6,7)
    //      ||+------- Upper two (2) colour bits for Square 2 (Tiles #8,9,A,B)
    //      +--------- Upper two (2) colour bits for Square 3 (Tiles #C,D,E,F)

    uint8_t byte_idx = (y/32)*8 + x / 32;
    uint8_t bit_idx = (y/16)*4 + (x/16)*2;
    uint8_t attr = (table[byte_idx] >> bit_idx) & 0x03;
    return attr << 2;
}


int ppu_init_sdl(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return -1;
    }
    win = SDL_CreateWindow("dNES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, W, H);
    return 0;
}

void ppu_write(uint8_t addr, uint8_t dat) {
    switch(addr) {
        case 0: // PPUCTRL, PPU Control Register #1
            ppu_ctrl1 = dat;
            break;
        case 1: // PPUMASK, PPU Control Register #2
            ppu_ctrl2 = dat;
            break;
        case 2: // PPUSTATUS, PPU Status Register
            break;
        case 3: // OAMADDR, SPR-RAM Address Register
            break;
        case 4: // OAMDATA, SPR-RAM I/O Register
            break;
        case 5: // PPUSCROLL, VRAM Address Register #1 (W2)
            break;
        case 6: // PPUADDR, VRAM Address Register #2 (W2)
            ppuaddr = ((ppuaddr & 0x3f) << 8) | dat;
            break;
        case 7: // PPUDATA, VRAM I/O Register
            vram[ppuaddr] = dat;
            ppuaddr += (ppu_ctrl1 & 0x04) ? 32 : 1;
            break;
        default:;
    }
}

uint8_t ppu_read(uint8_t addr) {
    uint8_t val = 0;
    switch(addr) {
        case 0: // PPUCTRL, PPU Control Register #1
            break;
        case 1: // PPUMASK, PPU Control Register #2
            break;
        case 2: // PPUSTATUS, PPU Status Register
            val = ppu_status;
            ppu_status &= 0x7f;
            break;
        case 3: // OAMADDR, SPR-RAM Address Register
            break;
        case 4: // OAMDATA, SPR-RAM I/O Register
            break;
        case 5: // PPUSCROLL, VRAM Address Register #1 (W2)
            break;
        case 6: // PPUADDR, VRAM Address Register #2 (W2)
            break;
        case 7: // PPUDATA, VRAM I/O Register
            val = vram[ppuaddr];
            ppuaddr += (ppu_ctrl1 & 0x04) ? 32 : 1;
            break;
        default:
            break;
    }
    return val;
}


void ppu_tick(void) {
    ppu_status |= 0x80;
    tick++;
}