#include "cartridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

cartridge_t cartridge = { 0 };

const uint8_t *cartridge_getCHR8k(uint8_t chr_idx) {
    assert(chr_idx < cartridge.header.nCHRROM8k);
    return cartridge.rom_chr8k + 8192 * chr_idx;
}

void cartridge_write(uint16_t addr, uint8_t dat) {
    switch(addr) {
        case 0x8000 ... 0xffff:
            break;
        default:;
    }
}

uint8_t cartridge_read(uint16_t addr) {
    uint8_t dat = 0;
    switch(addr) {
        case 0x8000 ... 0xbfff:
            dat = cartridge.rom_prg16k[addr - cartridge.header.nCHRROM8k == 1 ? 0xc000 : 0x8000];
        case 0xC000 ... 0xffff:
            dat = cartridge.rom_prg16k[addr - 0xc000];
            break;
        default:;
    }
    return dat;
}

void cartridge_loadROM(const char *fn) {
    FILE *f = fopen(fn, "r");
    fread(&cartridge.header, 1, sizeof(inesheader_t), f);
    printf("mapper %d\n", cartridge.header.mapperlo | ( cartridge.header.mapperhi << 4));
    printf("16k pages prg rom: %d\n", cartridge.header.nPRGROM16k);
    printf("8k pages chr rom: %d\n", cartridge.header.nCHRROM8k);
    cartridge.rom_prg16k = (uint8_t*)malloc(1024*16 * cartridge.header.nPRGROM16k);
    cartridge.rom_chr8k = (uint8_t*)malloc(1024*8 * cartridge.header.nCHRROM8k);
    fread(cartridge.rom_prg16k, cartridge.header.nPRGROM16k, 16*1024, f);
    fread(cartridge.rom_chr8k, cartridge.header.nCHRROM8k, 8*1024, f);
    fclose(f);
}

void cartridge_cleanup(void) {
    free(cartridge.rom_prg16k);
    free(cartridge.rom_chr8k);
}