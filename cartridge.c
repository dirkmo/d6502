#include "cartridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define NT_MIRROR_H (!cartridge.header.Vh)
#define NT_MIRROR_V (cartridge.header.Vh)

cartridge_t cartridge = { 0 };

static uint8_t mapper0_ppu_read(uint16_t addr) {
    extern uint8_t vram[0x4000];
    uint8_t val = 0;
    switch(addr) {
        case 0x0000 ... 0x1fff: // pattern table 1+2 ROM
            val = cartridge.rom_chr8k[addr];
            break;
        case 0x2000 ... 0x2fff: // nametable 0-3
            if (NT_MIRROR_V) { // $2000 = $2800, $2400 = $2C00
                addr &= ~0x0800;
            } else if (NT_MIRROR_H) { // $2000 = $2400, $2800 = $2C00
                addr &= ~0x0400;
            }
            val = vram[addr];
            break;
        case 0x3f00 ... 0x3fff: // palette RAM
            if((addr%4) == 0) {
                addr = 0x3f00;
            } else {
                addr &= 0xff1f;
            }
            val = vram[addr];
            break;
        default: assert(1);
    }
    return val;
}

static void mapper0_ppu_write(uint16_t addr, uint8_t dat) {
    extern uint8_t vram[0x4000];
    switch(addr) {
        case 0x0000 ... 0x1fff: // pattern table 1+2 ROM
            break;
        case 0x2000 ... 0x2fff: // nametable 0-3
            if (NT_MIRROR_V) { // $2000 = $2800, $2400 = $2C00
                addr &= ~0x0800;
            } else if (NT_MIRROR_H) { // $2000 = $2400, $2800 = $2C00
                addr &= ~0x0400;
            }
            vram[addr] = dat;
            break;
        case 0x3f00 ... 0x3fff: // palette RAM
            if((addr%4) == 0) {
                addr = 0x3f00;
            } else {
                addr &= 0xff1f;
            }
            vram[addr] = dat;
            break;
        default: assert(1);
    }
}

static void mapper0_cpu_write(uint16_t addr, uint8_t dat) {
    switch(addr) {
        case 0x8000 ... 0xffff:
            break;
        default: 
            assert(1);
    }
}

static uint8_t mapper0_cpu_read(uint16_t addr) {
    uint8_t dat = 0;
    switch(addr) {
        case 0x8000 ... 0xbfff:
            dat = cartridge.rom_prg16k[addr - 0x8000];
            break;
        case 0xC000 ... 0xffff:
            dat = cartridge.rom_prg16k[addr - 0xc000];
            break;
        default:;
    }
    return dat;
}

uint8_t cartridge_ppu_read(uint16_t addr) {
    return cartridge.mapper_ppu_read(addr % 0x4000);
}

void cartridge_ppu_write(uint16_t addr, uint8_t dat) {
    cartridge.mapper_ppu_write(addr % 0x4000, dat);
}

uint8_t cartridge_cpu_read(uint16_t addr) {
    return cartridge.mapper_cpu_read(addr);
}

void cartridge_cpu_write(uint16_t addr, uint8_t dat) {
    cartridge.mapper_cpu_write(addr, dat);
}

void cartridge_loadROM(const char *fn) {
    FILE *f = fopen(fn, "r");
    if (f == NULL) {
        printf("ERROR: File not found.\n");
        exit(1);
    }
    fread(&cartridge.header, 1, sizeof(inesheader_t), f);
    uint8_t mapper = cartridge.header.mapperlo | ( cartridge.header.mapperhi << 4);
    printf("16k pages prg rom: %d\n", cartridge.header.nPRGROM16k);
    printf("8k pages chr rom: %d\n", cartridge.header.nCHRROM8k);
    printf("mapper %d\n", mapper);
    if( cartridge.header.four ) {
        printf("No nametable mirroring, four-screen\n");
    } else {
        printf("%s nametable mirroring\n", NT_MIRROR_V ? "Vertical" : "Horizontal");
    }
    cartridge.rom_prg16k = (uint8_t*)malloc(1024*16 * cartridge.header.nPRGROM16k);
    cartridge.rom_chr8k = (uint8_t*)malloc(1024*8 * cartridge.header.nCHRROM8k);
    fread(cartridge.rom_prg16k, cartridge.header.nPRGROM16k, 16*1024, f);
    fread(cartridge.rom_chr8k, cartridge.header.nCHRROM8k, 8*1024, f);
    fclose(f);
    if (mapper == 0) {
        cartridge.mapper_ppu_read = mapper0_ppu_read;
        cartridge.mapper_ppu_write = mapper0_ppu_write;
        cartridge.mapper_cpu_read = mapper0_cpu_read;
        cartridge.mapper_cpu_write = mapper0_cpu_write;
    } else {
        printf("ERROR: Mapper %d not supported\n", mapper);
        exit(1);
    }
}

void cartridge_cleanup(void) {
    free(cartridge.rom_prg16k);
    free(cartridge.rom_chr8k);
}