// https://wiki.nesdev.com/w/index.php/PPU_rendering
//
#include "ppu.h"

#include <assert.h>
#include <stdio.h>
#include "nescolors.h"
#include "cartridge.h"

#define TICKS_PER_FRAME (TOTAL_FRAME_W * TOTAL_FRAME_H)
#define PATTERN_TABLE_0 0x0000
#define PATTERN_TABLE_1 0x1000
#define NAME_TABLE_0 0x2000

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

// ppu_status
#define VBLANK_MASK 0x80
#define SPRITE0HIT_MASK 0x40

#define SCROLL_X (ppu_scroll[0])
#define SCROLL_Y (ppu_scroll[1])

typedef struct {
    uint8_t y;
    uint8_t index;
    uint8_t attr;
    uint8_t x;
} sprite_t;

static uint32_t tick = 0;
/* static*/ uint32_t pixels[FRAME_W * FRAME_H];
static bool interrupt = false;
static bool sprite0hit = false;

uint8_t ppu_ctrl = 0;
uint8_t ppu_mask = 0;
uint8_t ppu_status = 0;
uint8_t ppu_scroll[2] = {0, 0};
uint16_t ppuaddr = 0;
uint8_t ppudata = 0;
uint8_t oam_addr = 0;

uint8_t vram[0x4000];

union {
    uint8_t raw[0x100];
    sprite_t sprite[0x100/4];
} oam;

uint8_t local_sprites[8];

void oam_collectSprites(uint8_t y) {
    int s = 0;
    for(int i = 0; i < 64 && s < 8; i++) {
        if ((y >= oam.sprite[i].y) && (y < oam.sprite[i].y+8)) {
            local_sprites[s++] = i;
        }
    }
    for (; s < 8; s++) {
        local_sprites[s] = 0xff;
    }
}

uint8_t oam_getSpriteIdxOnScanline(uint8_t x) {
    for(uint8_t i = 0; i<8; i++) {
        const sprite_t *sprite = &oam.sprite[local_sprites[i]];
        if( (x >= sprite->x) && (x < (sprite->x+8))) {
            // TODO: take sprite with first opaque pixel
            return local_sprites[i];
        }
    }
    return 0xff;
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
    uint8_t chr1 = cartridge_ppu_read(tile_addr + byteidx);
    uint8_t chr2 = cartridge_ppu_read(tile_addr + byteidx + 8);
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
    if ((color % 4) == 0) {
        color = 0; // backdrop color
    }
    uint16_t addr = 0x3f00 + color;
    pixels[p] = nescolors[vram[addr]];
}

uint8_t getBGTilePixel(uint16_t addr, uint8_t idx) {
    int bitidx = idx % 8;
    int byteidx = (idx / 8);
    uint8_t chr1 = cartridge_ppu_read(addr+byteidx);
    uint8_t chr2 = cartridge_ppu_read(addr+byteidx+8);
    uint8_t b1 = (chr1 >> (7-bitidx)) & 1;
    uint8_t b2 = (chr2 >> (7-bitidx)) & 1;
    return b1 | (b2 << 1);
}

const uint16_t getBGTileAddr(uint8_t idx) {
    uint16_t base = BG_PATTERN_TABLE_SEL ? PATTERN_TABLE_1 : PATTERN_TABLE_0;
    return base + 16 * idx;
}

uint8_t getAttribute(uint16_t attrtable_baseaddr, uint8_t x, uint8_t y) {
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
    uint16_t addr = attrtable_baseaddr + byte_idx;
    assert( addr < sizeof(vram));
    uint8_t attr = (cartridge_ppu_read(addr) >> bit_idx) & 0x03;
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
            ppu_scroll[0] = ppu_scroll[1];
            ppu_scroll[1] = dat;
            break;
        case 6: // PPUADDR, VRAM Address Register #2 (W2)
            ppuaddr = ((ppuaddr & 0x3f) << 8) | dat;
            break;
        case 7: // PPUDATA, VRAM I/O Register
            cartridge_ppu_write(ppuaddr, dat);
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
            ppu_status &= ~VBLANK_MASK;
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
            val = cartridge_ppu_read(ppuaddr);
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
    return cartridge_ppu_read(addr);
}

static uint8_t pixel_prio(uint8_t sprite_idx, uint8_t sprcol, uint8_t bgcol) {
    // attr bit 5: Sprite priority (0: in front of background; 1: behind background)
    // col % 4 == 0 --> transparent pixel (backdrop color)
    if (sprite_idx < 64) {
        if (((oam.sprite[sprite_idx].attr & 0x20) == 0) && ((sprcol % 4) != 0)) {
            return sprcol;
        }
    }
    return bgcol;
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

void sprite0hit_handler(int x, int y, uint8_t bgcol, uint8_t sprcol) {
    if (!sprite0hit) {
        if (SHOW_BG_ENABLED && SHOW_SPRITES_ENABLED) {
            if (x > 7 || SPRITES_LEFT_ENABLED || BG_LEFT_ENABLED) {
                ppu_status |= SPRITE0HIT_MASK;
                sprite0hit = true;
            }
        }
    }
}

#define ATTR_TABLE_BASE(ntaddr) ((ntaddr & 0x2c00) + 0x3c0)

void blitBGLine(uint8_t y, uint8_t *line) {
    int line_x = 0;
    uint16_t ntoffsy = (y / 8) * 32 + SCROLL_X / 8;
    uint16_t ntaddr = getNameTableAddr() + ntoffsy;
    uint16_t attroffsy = (y / 32) * 8 + SCROLL_X / 32;
    uint16_t attraddr = ATTR_TABLE_BASE(ntaddr) + attroffsy;
    uint8_t attr = 0;
    uint8_t attrbits = 0;
    uint8_t chr1 = 0;
    uint8_t chr2 = 0;
    for ( int x = SCROLL_X; x < SCROLL_X + FRAME_W; x++) {
        if (x == FRAME_W) {
            // NT switch for horiz scroll --> toggle bit 10
            ntaddr &= 0x2c00;
            ntaddr = (ntaddr & 0x400) ? (ntaddr & ~0x400) : (ntaddr | 0x400) + ntoffsy;
            attraddr = ATTR_TABLE_BASE(ntaddr) + attroffsy;
        }
        if (x % 8 == 0) {
            // fetch tile
            uint8_t tile_idx = cartridge_ppu_read(ntaddr++);
            uint16_t tile_addr = getBGTileAddr(tile_idx);
            chr1 = cartridge_ppu_read(tile_addr + (y % 8));
            chr2 = cartridge_ppu_read(tile_addr + (y % 8) + 8);
            if (x % 16 == 0) {
                if (x % 32 == 0) {
                    // fetch attribute
                    attr = cartridge_ppu_read(attraddr++);
                }
                uint8_t attrbit_idx = (x % 32) > 15 ? 2 : 0;
                attrbit_idx += (y % 32) > 15 ? 4 : 0;
                attrbits = ((attr >> attrbit_idx) & 0x03) << 2;
            }
        }
        int bitidx = 7 - (x % 8);
        line[line_x++] = ((chr1 >> bitidx) & 1) | (((chr2 >> bitidx) & 1) << 1) | attrbits;
    }
}

int blitSpriteLine(uint8_t y, uint8_t *line) {
    const uint16_t ptbase = SPRITE_PATTERN_TABLE_SEL ? PATTERN_TABLE_1 : PATTERN_TABLE_0;
    int s0_hit_pos = -1;
    oam_collectSprites(y);
    for (int t = 0; t < 8; t++) {
        uint8_t sprite_idx = local_sprites[t];
        if (sprite_idx == 0xff) {
            continue;
        }
        const sprite_t *sprite = &oam.sprite[sprite_idx];
        if (y >= sprite->y && y < sprite->y + 8) {
            int ty = y - sprite->y;
            if (sprite->attr & 0x80) {
                 ty = 7 - ty; // flip y
            }
            uint16_t addr = ptbase + sprite->index * 16 + ty;
            uint8_t chr1 = cartridge_ppu_read(addr);
            uint8_t chr2 = cartridge_ppu_read(addr + 8);
            for (int x = 0; x < 8; x++) {
                int bitidx = (sprite->attr & 0x40) ? x : (7 - x); // flip x
                uint8_t sprcol = 0x10 | ((chr1 >> bitidx) & 1) | (((chr2 >> bitidx) & 1) << 1) | ((sprite->attr & 3) << 2);
                if (!(sprite->attr & 0x20) && (sprcol % 4)) {
                    line[sprite->x + x] = sprcol;
                    if ((local_sprites[t] == 0) && (s0_hit_pos < 0)) {
                        s0_hit_pos = sprite->x + x;
                    }
                }
            }
        }
    }
    return s0_hit_pos;
}


void ppu_tick(void) {
    static uint8_t scanline[FRAME_W];

    uint32_t frame_pixel_idx = tick % TICKS_PER_FRAME;
    uint32_t y = frame_pixel_idx / TOTAL_FRAME_W;
    uint32_t x = frame_pixel_idx % TOTAL_FRAME_W;

    uint8_t relx = x % 8;
    uint8_t rely = y % 8;

    if (x == 0) {
        // beginning of line
        if (y == 0) {
            // beginning of frame
            ppu_status &= ~VBLANK_MASK;
            ppu_status &= ~SPRITE0HIT_MASK;
            sprite0hit = false;
            interrupt = false;
        }
        if (y < FRAME_H) {
            blitBGLine(y, scanline);
            int hit_xpos = blitSpriteLine(y, scanline);
            if (hit_xpos > -1) {
                // sprite0hit
            }
        }
    } else {
        if (x == TOTAL_FRAME_W-1) {
            if (y == FRAME_H-1) {
                // last visible line done
                ppu_status |= VBLANK_MASK;
                interrupt = true;
            }
        }
    }

    if (x < FRAME_W) {
        if (y < FRAME_H) {
            // Visible pixels
            setpixel(x, y, scanline[x]);
        }
    } else {
        // HBLANK
        if(y < FRAME_H) {
            oam_addr = 0;
        }
    }
    tick++;
}
