#include "cartridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

cartridge_t cartridge = { 0 };

uint8_t mapper0_ppu_read(uint16_t addr) {
    assert(addr < 8*1024);
    return cartridge.rom_chr8k[addr];
}

void mapper0_ppu_write(uint16_t addr, uint8_t dat) {

}

void mapper0_cpu_write(uint16_t addr, uint8_t dat) {
    switch(addr) {
        case 0x8000 ... 0xffff:
            break;
        default:;
    }
}

uint8_t mapper0_cpu_read(uint16_t addr) {
    uint8_t dat = 0;
    switch(addr) {
        case 0x8000 ... 0xbfff:
            dat = cartridge.rom_prg16k[addr - 0x8000];
        case 0xC000 ... 0xffff:
            dat = cartridge.rom_prg16k[addr - 0xc000];
            break;
        default:;
    }
    return dat;
}

uint8_t cartridge_ppu_read(uint16_t addr) {
    return cartridge.mapper_ppu_read(addr);
}

void cartridge_ppu_write(uint16_t addr, uint8_t dat) {
    cartridge.mapper_ppu_write(addr, dat);
}

uint8_t cartridge_cpu_read(uint16_t addr) {
    return cartridge.mapper_cpu_read(addr);
}

void cartridge_cpu_write(uint16_t addr, uint8_t dat) {
    cartridge.mapper_cpu_write(addr, dat);
}

void cartridge_loadROM(const char *fn) {
    FILE *f = fopen(fn, "r");
    fread(&cartridge.header, 1, sizeof(inesheader_t), f);
    uint8_t mapper = cartridge.header.mapperlo | ( cartridge.header.mapperhi << 4);
    printf("mapper %d\n", mapper);
    printf("16k pages prg rom: %d\n", cartridge.header.nPRGROM16k);
    printf("8k pages chr rom: %d\n", cartridge.header.nCHRROM8k);
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
        exit(1);
    }
}

void cartridge_cleanup(void) {
    free(cartridge.rom_prg16k);
    free(cartridge.rom_chr8k);
}