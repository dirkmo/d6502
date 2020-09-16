#include "d6502.h"
#include "instruction_table.h"
#include "ppu.h"
#include "cartridge.h"
#include "inesheader.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

uint8_t memory[0x10000];

uint8_t ram_internal[0x800];


int EMULATION_END = 0;
uint32_t run_count = 0;
uint16_t breakpoint = 0;


void writebus(uint16_t addr, uint8_t dat) {
    switch(addr) {
        case 0x0000 ... 0x1fff: // internal ram
            ram_internal[addr & 0x7ff] = dat;
            break;
        case 0x2000 ... 0x3fff: // PPU
            ppu_write(addr & 0x7, dat);
            break;
        case 0x4000 ... 0x401f: // APU + IO
            break;
        case 0x6000 ... 0xffff: // cartridge
            cartridge_write(addr, dat);
            break;
        default:;
    }
}

uint8_t readbus(uint16_t addr) {
    switch(addr) {
        case 0x0000 ... 0x1fff: // internal ram
            return ram_internal[addr];
        case 0x2000 ... 0x3fff: // PPU
            return ppu_read(addr & 0x7);
        case 0x4000 ... 0x401f: // APU + IO
            return 0;
        case 0x6000 ... 0xffff: // cartridge
            return cartridge_read(addr);
        default: ;
    }
    return 0;
}

void print_regs(d6502_t *cpu) {
    char status[32];
    sprintf(status, "st: %02X (%c%c-%c%c%c%c%c)", cpu->st,
        cpu->st & 0x80 ? 'N' : 'n',
        cpu->st & 0x40 ? 'V' : 'v',
        cpu->st & 0x10 ? 'B' : 'b',
        cpu->st & 0x08 ? 'D' : 'd',
        cpu->st & 0x04 ? 'I' : 'i',
        cpu->st & 0x02 ? 'Z' : 'z',
        cpu->st & 0x01 ? 'C' : 'c');
    printf("A: %02X, X: %02X, Y: %02X, SP: %02X, PC: %02X, %s\n", cpu->a, cpu->x, cpu->y, cpu->sp, cpu->pc, status);
}

void memory_dump(uint16_t addr, int cols, int rows) {
    for (int r = 0; r < rows; r++ ) {
        printf("$%04X: ", addr + r*cols);
        for (int c = 0; c < cols; c++) {
            printf("%02X ", memory[addr + r*cols + c]);
        }
        printf("\n");
    }
}

int read_line(char *s, int maxlen) 
{
    int ch;
    int i = 0;
    while( (ch = getchar()) != '\n' && ch != EOF && i < maxlen-1) {
        s[i++] = ch;
    }
    s[i] = 0;
    return i;
}

void handle(d6502_t *cpu, const char *cmd) {
    unsigned int addr;
    if (strcmp(cmd, "exit") == 0) {
        EMULATION_END = 1;
    } else if(strstr(cmd, "dump") != 0 && sscanf(cmd, "dump %x", &addr) == 1) {
        memory_dump(addr, 16, 8);
    } else if(strstr(cmd, "regs")) {
        print_regs(cpu);
    } else if(strstr(cmd, "run") != 0 ) {
        int argc = sscanf(cmd, "run %d", &addr);
        if (argc == 1) {
            run_count = addr;
        } else {
            run_count = 0xFFFFffff;
        }
    } else if(strstr(cmd, "break") != 0 && sscanf(cmd, "break %x", &addr) == 1) {
        breakpoint = addr;
    } else if(strlen(cmd) > 0) {
        printf("Unknown command '%s'\n", cmd);
    }
}

void get_raw_instruction(d6502_t *cpu, char raw[]) {
    char temp[10];
    uint8_t dat = cpu->read(cpu->pc);
    sprintf(raw, "%02X", dat);
    const instruction_t *inst = get_instruction(dat);
    for( int i = 1; i < 3/*inst->len*/; i++ ) {
        dat = cpu->read(cpu->pc + i);
        if( i < inst->len)
            sprintf(temp, " %02X", dat);
        else
            sprintf(temp, "   ");
        strcat(raw, temp);
    }
}

void onExit(void) {
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    if(ppu_init_sdl() < 0) {
        return 1;
    }
    atexit(onExit);
    cartridge_loadROM("rom/bg.nes");

    // bool quit = false;
    // SDL_Event e;
    // while(!quit) {
    //     while(SDL_PollEvent(&e)) {
    //         if (e.type == SDL_QUIT) {
    //             quit = true;
    //         }
    //         if (e.type == SDL_KEYDOWN) {
    //         }
    //         if (e.type == SDL_WINDOWEVENT ) {
    //             if (e.window.event == SDL_WINDOWEVENT_RESIZED || e.window.event == SDL_WINDOWEVENT_SHOWN) {
    //                 draw();
    //             }
    //             if (e.window.event == SDL_WINDOWEVENT_EXPOSED ) {
    //                 draw();
    //             }
    //             if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
    //                 quit = true;
    //             }
    //         }
    //     }
    // }

    d6502_t cpu;
    d6502_init(&cpu);
    cpu.read = readbus;
    cpu.write = writebus;
    
    d6502_reset(&cpu);

    FILE *log = fopen("log.txt", "w");

    int instruction_counter = 1;
    char asmcode[32];
    char raw[16];
    char logstr[128];
    char buf[256];
    while( EMULATION_END == 0) {
        d6502_disassemble(&cpu, cpu.pc, asmcode);
        get_raw_instruction(&cpu, raw);
        print_regs(&cpu);
        do {
            printf("\n%d $%04X: %s   %s> ", instruction_counter, cpu.pc, raw, asmcode);
            if (breakpoint == 0) {
                if( instruction_counter < run_count ) break;
            } else if (breakpoint != cpu.pc) {
                break;
            } else {
                breakpoint = 0;
            }
            buf[0] = 0;
            read_line(buf, sizeof(buf));
            handle(&cpu, buf);
        } while(strlen(buf) > 0);
        printf("\n");
        sprintf(logstr, "%04X  %s  %s", cpu.pc, raw, asmcode);
        int p = strlen(logstr);
        while( p < 48 ) {
            logstr[p++] = ' ';
        }
        sprintf(logstr+p, "A:%02X X:%02X Y:%02X P:%02X SP:%02X\n", cpu.a, cpu.x, cpu.y, cpu.st, cpu.sp);
        fwrite(logstr, strlen(logstr), 1, log);
        fflush(log);

        int clock = 0;
        while(1) {
            // ppu runs 3x faster than the cpu
            ppu_tick();
            clock++;
            if((clock%3) == 0) {
                // execute instruction
                if( d6502_tick(&cpu) == 0 ) {
                    break;
                }
            }
        }
        instruction_counter++; // instruction counter
    }
    fclose(log);

    return 0;
}
