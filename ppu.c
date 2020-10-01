// https://wiki.nesdev.com/w/index.php/PPU_rendering
//
#include "ppu.h"

#include <assert.h>
#include <stdio.h>
#include "nescolors.h"
#include "cartridge.h"

#define VBLANK 0x80
#define TICKS_PER_FRAME (TOTAL_FRAME_W * TOTAL_FRAME_H)
#define PATTERN_TABLE_0 0x0000
#define PATTERN_TABLE_1 0x1000
#define NAME_TABLE_0 0x2000
#define NAME_TABLE_1 0x2400
#define NAME_TABLE_2 0x2800
#define NAME_TABLE_3 0x2c00
#define ATTR_TABLE_0 (NAME_TABLE_0 + 0x3c0)
#define ATTR_TABLE_1 (NAME_TABLE_1 + 0x3c0)
#define ATTR_TABLE_2 (NAME_TABLE_2 + 0x3c0)
#define ATTR_TABLE_3 (NAME_TABLE_3 + 0x3c0)

#define BACKGROUND_COLOR (vram[0x3f00])

// ppu_ctrl register
#define SPRITE_PATTERN_TABLE_SEL    (ppu_ctrl & 0x08)
#define BG_PATTERN_TABLE_SEL        (ppu_ctrl & 0x10)
#define SPRITE_SIZE_8x16            (ppu_ctrl & 0x20)

// ppu_mask register
#define COLOR_ENABLED           (ppu_mask & 0x01)
#define BG_LEFT_ENABLED         (ppu_mask & 0x02)
#define SPRITES_LEFT_ENABLED    (ppu_mask & 0x04)
#define SHOW_BG_ENABLED         (ppu_mask & 0x08)
#define SHOW_SPRITES_ENABLED    (ppu_mask & 0x10)
#define EMPHASIZE_RED_ENABLED   (ppu_mask & 0x20)
#define EMPHASIZE_GREEN_ENABLED (ppu_mask & 0x40)
#define EMPHASIZE_BLUE_ENABLED  (ppu_mask & 0x80)

typedef struct {
    uint8_t y;
    uint8_t index;
    uint8_t attr;
    uint8_t x;
} sprite_t;

static uint32_t tick = 0;
/* static*/ uint32_t pixels[FRAME_W * FRAME_H];
static bool interrupt = false;

uint8_t ppu_ctrl = 0;
uint8_t ppu_mask = 0;
uint8_t ppu_status = 0;
uint16_t ppuaddr = 0;
uint8_t ppudata = 0;
uint8_t oam_addr = 0;

uint8_t vram[0x4000];

union {
    uint8_t raw[0x100];
    sprite_t sprite[0x100/4];
} oam;

sprite_t local_sprites[8];

void oam_collectSprites(uint8_t y) {
    int s = 0;
    for(int i = 0; i < 64 && s < 8; i++) {
        if ((y >= oam.sprite[i].y) && (y < oam.sprite[i].y+8)) {
            local_sprites[s++] = oam.sprite[i];
        }
    }
    for (; s < 8; s++) {
        local_sprites[s] = (sprite_t){ 0xff, 0xff, 0xff, 0xff };
    }
}

const sprite_t *oam_getSprite(uint8_t x) {
    for(int i = 0; i<8; i++) {
        if( (x >= local_sprites[i].x) && (x < (local_sprites[i].x+8))) {
            return &local_sprites[i];
        }
    }
    return NULL;
}

const uint16_t get_spriteTileAddr(uint8_t idx) {
    uint16_t tableaddr;
    if (SPRITE_SIZE_8x16) {
        // 8x16 sprites
        tableaddr = (idx & 1) ? PATTERN_TABLE_1 : PATTERN_TABLE_0;
        idx = idx & 0xFE; // bottom half of sprite follows
        assert(1);
    } else {
        // 8x8 sprites
        tableaddr = SPRITE_PATTERN_TABLE_SEL ? PATTERN_TABLE_1 : PATTERN_TABLE_0;
    }
    return tableaddr + 16 * idx;
}

uint8_t getSpriteTilePixel(uint16_t tile_addr, uint8_t x, uint8_t y, uint8_t attr) {
    if ((attr & 0x40)) {
        // flip horizontally
        x = 7 - x;
    }
    if ((attr & 0x80)) {
        // flip vertically
        y = 7 - y;
    }
    int bitidx = x;
    int byteidx = y;
    uint8_t chr1 = cartridge.readPatternTable(tile_addr + byteidx);
    uint8_t chr2 = cartridge.readPatternTable(tile_addr + byteidx + 8);
    uint8_t b1 = (chr1 >> (7-bitidx)) & 1;
    uint8_t b2 = (chr2 >> (7-bitidx)) & 1;
    return b1 | (b2 << 1);
}

const uint32_t *ppu_getFrameBuffer(void) {
    return &pixels[0];
}

void setpixel( int x, int y, uint8_t color ) {
    int p = y * FRAME_W + x;
    if( p >= sizeof(pixels)) {
        printf("x: %d, y: %d\n", x, y);
        assert(p < sizeof(pixels));
    }
    pixels[p] = nescolors[vram[0x3f00+color]];
}

uint8_t getBGTilePixel(uint16_t addr, uint8_t idx) {
    int bitidx = idx % 8;
    int byteidx = (idx / 8);
    uint8_t chr1 = cartridge.readPatternTable(addr+byteidx);
    uint8_t chr2 = cartridge.readPatternTable(addr+byteidx+8);
    uint8_t b1 = (chr1 >> (7-bitidx)) & 1;
    uint8_t b2 = (chr2 >> (7-bitidx)) & 1;
    return b1 | (b2 << 1);
}

// void drawBGTile( const uint8_t *rawtile, const uint8_t *colors, int x, int y) {
//     for ( int i = 0; i < 64; i++ ) {
//         uint8_t color = colors[getBGTilePixel(rawtile, i)] * 3;
//         setpixel(x+i%8, y+i/8, color);
//     }
// }

const uint16_t getBGTileAddr(uint8_t idx) {
    uint16_t base = BG_PATTERN_TABLE_SEL ? PATTERN_TABLE_1 : PATTERN_TABLE_0;
    return base + 16 * idx;
}

uint8_t getAttribute(uint16_t nametable_baseaddr, uint8_t x, uint8_t y) {
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
    //      ||||||+--- Upper two (2) colour bits for Square 0 (Tiles #0,1,2,3); (x % 32) < 16; (y % 32) < 16; lower nibble bits 0+1
    //      ||||+----- Upper two (2) colour bits for Square 1 (Tiles #4,5,6,7); (x % 32) > 15; (y % 32) < 16; lower nibble bits 2+3
    //      ||+------- Upper two (2) colour bits for Square 2 (Tiles #8,9,A,B); (x % 32) < 16; (y % 32) > 15; upper nibble bits 0+1
    //      +--------- Upper two (2) colour bits for Square 3 (Tiles #C,D,E,F); (x % 32) > 15; (y % 32) > 15; upper nibble bits 2+3

    int byte_idx = (y/32)*8 + x/32;
    uint8_t bit_idx = ((y & 0x10) >> 2) + ((x & 0x10) >> 3);
    uint16_t addr = nametable_baseaddr + byte_idx;
    assert( addr < sizeof(vram));
    uint8_t attr = (vram[addr] >> bit_idx) & 0x03;
    return attr << 2;
}

void ppu_write(uint8_t addr, uint8_t dat) {
    switch(addr) {
        case 0: // PPUCTRL, PPU Control Register #1
            ppu_ctrl = dat;
            break;
        case 1: // PPUMASK, PPU Control Register #2
            ppu_mask = dat;
            break;
        case 2: // PPUSTATUS, PPU Status Register
            break;
        case 3: // OAMADDR, SPR-RAM Address Register
            oam_addr = dat;
            break;
        case 4: // OAMDATA, SPR-RAM I/O Register
            oam.raw[oam_addr++] = dat;
            break;
        case 5: // PPUSCROLL, VRAM Address Register #1 (W2)
            break;
        case 6: // PPUADDR, VRAM Address Register #2 (W2)
            ppuaddr = ((ppuaddr & 0x3f) << 8) | dat;
            break;
        case 7: // PPUDATA, VRAM I/O Register
            vram[ppuaddr] = dat;
            ppuaddr += (ppu_ctrl & 0x04) ? 32 : 1;
            break;
        default:;
    }
}

uint8_t ppu_read(uint8_t addr) {
    uint8_t val = 0;
    switch(addr) {
        case 0: // PPUCTRL, PPU Control Register #1
            val = ppu_ctrl;
            break;
        case 1: // PPUMASK, PPU Control Register #2
            val = ppu_mask;
            break;
        case 2: // PPUSTATUS, PPU Status Register
            val = ppu_status;
            ppu_status &= ~VBLANK;
            break;
        case 3: // OAMADDR, SPR-RAM Address Register
            break;
        case 4: // OAMDATA, SPR-RAM I/O Register
            val = oam.raw[oam_addr];
            break;
        case 5: // PPUSCROLL, VRAM Address Register #1 (W2)
            break;
        case 6: // PPUADDR, VRAM Address Register #2 (W2)
            val = ppuaddr;
            break;
        case 7: // PPUDATA, VRAM I/O Register
            val = vram[ppuaddr];
            ppuaddr += (ppu_ctrl & 0x04) ? 32 : 1;
            break;
        default:
            break;
    }
    return val;
}

uint16_t getNameTableAddr(void) {
    uint16_t addr = NAME_TABLE_0 | ((ppu_ctrl & 0x3) << 10);
    return addr;
}

uint16_t getAttributeTableAddr(void) {
    return getNameTableAddr() + 0x3c0;
}

uint8_t getNameTableEntry(uint16_t nametable_baseaddr, int x, int y) {
    x = x / 8;
    y = y / 8;
    uint16_t idx = y * 32 + x;
    uint16_t addr = nametable_baseaddr + idx;
    assert(addr < sizeof(vram));
    return vram[addr];
}

bool ppu_interrupt(void) {
    return (ppu_ctrl & 0x80) && interrupt;
}

bool ppu_should_draw(void) {
    static uint32_t last = 0;
    if (tick > last + TICKS_PER_FRAME) {
        last = tick;
        return true;
    }
    return false;
}

void ppu_tick(void) {
    static uint16_t bgtile_addr;
    static uint8_t attr = 0;

    uint32_t frame_pixel_idx = tick % TICKS_PER_FRAME;
    uint32_t y = frame_pixel_idx / TOTAL_FRAME_W;
    uint32_t x = frame_pixel_idx % TOTAL_FRAME_W;

    uint8_t relx = x % 8;
    uint8_t rely = y % 8;

    if (x == 0) {
        // beginning of line
        oam_collectSprites(y);
        if ( y == 0 ) {
            // beginning of frame
            ppu_status &= ~VBLANK;
            interrupt = false;
        }
    } else {
        if (x == TOTAL_FRAME_W-1) {
            if (y == FRAME_H-1) {
                // last visible line done
                ppu_status |= VBLANK;
                interrupt = true;
            }
        }
    }

    if (x < FRAME_W) {
        if (y < FRAME_H) {
            // Visible pixels
            uint8_t bgpixel = 0;
            if (SHOW_BG_ENABLED) {
                if( BG_LEFT_ENABLED || x >= 8) {
                    if (relx == 0) {
                        // upperleft of tile
                        uint8_t tile_idx = getNameTableEntry(getNameTableAddr(), x, y);
                        bgtile_addr = getBGTileAddr(tile_idx);
                        if (x % 16 == 0) {
                            attr = getAttribute( getAttributeTableAddr(), x, y);
                        }
                    }
                    bgpixel = SHOW_BG_ENABLED ? (attr | getBGTilePixel(bgtile_addr, 8*rely+relx)) : 0;
                }
            }
            int sprite_pixel = -1;
            if (SHOW_SPRITES_ENABLED) {
                if( SPRITES_LEFT_ENABLED || x >= 8) {
                    const sprite_t *sprite = oam_getSprite(x);
                    if (sprite) {
                        uint16_t sprite_tile_addr = get_spriteTileAddr(sprite->index);
                        sprite_pixel = getSpriteTilePixel(sprite_tile_addr, x - sprite->x, y - sprite->y, sprite->attr);
                    }
                }
            }
            uint8_t pixel = (sprite_pixel > 0) ? sprite_pixel : bgpixel;
            setpixel(x, y, pixel);
        }
    } else {
        // HBLANK
        if(y < FRAME_H) {
            oam_addr = 0;
        }
    }
    tick++;
}
