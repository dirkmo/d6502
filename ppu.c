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
    uint8_t b2 = (rawtile[byteidx+1] >> (7-bitidx)) & 1;
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
    uint8_t colors[] = { 5, 0x10, 0x26, 0x30};
    const uint8_t *tile;
    for( int i = 0; i < 256; i++ ) {
        tile = getTile(0, i);
        drawTile(tile, colors, (i%16)*8, (i/16)*8);
    }

    SDL_UpdateTexture(tex, NULL, pixels, W*3);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
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
}

uint8_t ppu_read(uint8_t addr) {
    return 0;
}
