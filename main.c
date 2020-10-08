#include "d6502.h"
#include "instruction_table.h"
#include "ppu.h"
#include "apu.h"
#include "cartridge.h"
#include "inesheader.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>


#define WIN_W 800
#define WIN_H 600

static SDL_Window *win = NULL;
static SDL_Renderer *ren = NULL;
static SDL_Texture *tex = NULL;

struct dma_t {
    uint8_t page;
    int count; // if < 0 then no dma active
} dma = { .page = 2, .count = -1 };


uint8_t memory[0x10000];

uint8_t ram_internal[0x800];

// #define printf(...) (void)0;

int EMULATION_END = 0;
uint32_t run_count = 0;
uint16_t breakpoint = 0;

int init_sdl(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return -1;
    }
    win = SDL_CreateWindow("dNES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, FRAME_W*4, FRAME_H*4, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, FRAME_W, FRAME_H);
    return 0;
}

void draw(void) {
    SDL_UpdateTexture(tex, NULL, ppu_getFrameBuffer(), FRAME_W*4);
    // const SDL_Rect dst = {.x = 0, .y = 0, .w = FRAME_W, .h = FRAME_H };
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
 }


void writebus(uint16_t addr, uint8_t dat) {
    switch(addr) {
        case 0x0000 ... 0x1fff: // internal ram
            ram_internal[addr & 0x7ff] = dat;
            break;
        case 0x2000 ... 0x3fff: // PPU
            ppu_write(addr & 0x7, dat);
            break;
        case 0x4014:
            dma.count = 0;
            dma.page = dat;
            break;
        case 0x4000 ... 0x4013: // APU + IO
        case 0x4015 ... 0x401f: // APU + IO
            apu_write(addr & 0x1f, dat );
            break;
        case 0x6000 ... 0xffff: // cartridge
            cartridge_cpu_write(addr, dat);
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
            return apu_read(addr & 0x1f);
        case 0x6000 ... 0xffff: // cartridge
            return cartridge_cpu_read(addr);
        default: ;
    }
    return 0;
}

void dma_handler(void) {
    if (dma.count < 256) {
        uint8_t dat = readbus(dma.page * 256 + dma.count++);
        ppu_write(4, dat);
    } else {
        dma.count = -1;
    }
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
    if(init_sdl() < 0) {
        return 1;
    }
    draw();
    atexit(onExit);
    // cartridge_loadROM("rom/Tetris.nes");
    // cartridge_loadROM("rom/nestest.nes");
    // cartridge_loadROM("rom/Ice Climber (USA, Europe).nes");
    // cartridge_loadROM("rom/Pac-Man (USA).nes");
    // cartridge_loadROM("rom/Balloon Fight (USA).nes");
    // cartridge_loadROM("rom/DonkeyKong.nes");
    cartridge_loadROM("rom/LodeRunnerUSA.nes");

    d6502_t cpu;
    d6502_init(&cpu);
    cpu.read = readbus;
    cpu.write = writebus;
    
    d6502_reset(&cpu);
    
#ifdef FILELOG
    FILE *log = NULL;
    log = fopen("log.txt", "w");
    char logstr[128];
#endif
    int instruction_counter = 1;
    char asmcode[32];
    char raw[16];
    char buf[256];
    SDL_Event e;
    int nmi_count = 0;
    while( EMULATION_END == 0) {

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                EMULATION_END = 1;
            }
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        apu_report_buttonpress(BUTTON_UP, e.type == SDL_KEYDOWN);
                        break;
                    case SDLK_DOWN:
                        apu_report_buttonpress(BUTTON_DOWN, e.type == SDL_KEYDOWN);
                        break;
                    case SDLK_LEFT:
                        apu_report_buttonpress(BUTTON_LEFT, e.type == SDL_KEYDOWN);
                        break;
                    case SDLK_RIGHT:
                        apu_report_buttonpress(BUTTON_RIGHT, e.type == SDL_KEYDOWN);
                        break;
                    case SDLK_y:
                        apu_report_buttonpress(BUTTON_A, e.type == SDL_KEYDOWN);
                        break;
                    case SDLK_x:
                        apu_report_buttonpress(BUTTON_B, e.type == SDL_KEYDOWN);
                        break;
                    case SDLK_c:
                        apu_report_buttonpress(BUTTON_SELECT, e.type == SDL_KEYDOWN);
                        break;
                    case SDLK_v:
                        apu_report_buttonpress(BUTTON_START, e.type == SDL_KEYDOWN);
                        break;
                    case SDLK_ESCAPE:
                        EMULATION_END = 1;
                        break;
                    default: ;
                }
            }
            if (e.type == SDL_WINDOWEVENT ) {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED || e.window.event == SDL_WINDOWEVENT_SHOWN) {
                }
                if (e.window.event == SDL_WINDOWEVENT_EXPOSED ) {
                }
                if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                    EMULATION_END = 1;
                }
            }
        } // while (SDL_PollEvent(&e))

        d6502_disassemble(&cpu, cpu.pc, asmcode);

#if 0
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
#ifdef FILELOG
        sprintf(logstr, "%04X  %s  %s", cpu.pc, raw, asmcode);
        int p = strlen(logstr);
        while( p < 48 ) {
            logstr[p++] = ' ';
        }
        sprintf(logstr+p, "A:%02X X:%02X Y:%02X P:%02X SP:%02X\n", cpu.a, cpu.x, cpu.y, cpu.st, cpu.sp);
        fwrite(logstr, strlen(logstr), 1, log);
        fflush(log);
#endif
#endif
        int clock = 0;
        while(1) {
            // ppu runs 3x faster than the cpu
            ppu_tick();
            if( ppu_should_draw() ) {
                draw();
            }
            if (dma.count < 0) {
                if((clock%3) == 0) {
                    // execute instruction
                    if( d6502_tick(&cpu) == 0 ) {
                        break;
                    }
                }
            } else {
                dma_handler();
            }
            clock++;
        }
        instruction_counter++; // instruction counter
        if (ppu_interrupt()) {
            if(nmi_count == 0) {
                d6502_nmi(&cpu);
            }
            nmi_count++;
        } else {
            nmi_count = 0;
        }
    }
#ifdef FILELOG
    fclose(log);
#endif

    return 0;
}
