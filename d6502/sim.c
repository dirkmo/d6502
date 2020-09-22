#include "d6502.h"
#include "instruction_table.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

uint8_t memory[0x10000];

int EMULATION_END = 0;
uint32_t run_count = 0;
uint16_t breakpoint = 0;
bool nmi = false;
bool intr = false;

void writebus(uint16_t addr, uint8_t dat) {
    switch(addr) {
        default: memory[addr] = dat;
    }
}

uint8_t readbus(uint16_t addr) {
    switch(addr) {
        default: return memory[addr];
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
    } else if(strstr(cmd, "nmi") != 0) {
        printf("nmi triggered\n"),
        nmi = true;
    } else if(strstr(cmd, "int") != 0) {
        printf("interrupt triggered\n");
        intr = true;
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

int load(uint16_t addr, const char *fn) {
    FILE *f = fopen(fn, "r");
    if (f == NULL) {
        printf("Failed to open file\n");
        return 0;
    }
    const int readsize = 256;
    size_t s = readsize;
    uint16_t count = 0;
    while (s == readsize) {
        if (addr+readsize >= sizeof(memory)) {
            printf("out of memory!\n");
            return 2;
        } 
        s = fread(&memory[addr], 1, readsize, f);
        addr += s;
        count += s;
    }
    fclose(f);
    printf("%d bytes read\n", count);
    return 1;
}

void write16(uint16_t addr, uint16_t dat) {
    memory[addr] = dat;
    memory[addr+1] = dat >> 8;
}

int main(int argc, char *argv[]) {
    if (!load(0x1000, "test.bin")) {
        return 1;
    }
    d6502_t cpu;
    d6502_init(&cpu);
    cpu.read = readbus;
    cpu.write = writebus;
    
    write16(RESET_ADDR, 0x1000);
    write16(NMI_ADDR, 0x1008);
    write16(INT_ADDR, 0x100f);
    
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

        if (intr) {
            d6502_interrupt(&cpu);
            intr = false;
        }
        if (nmi) {
            d6502_nmi(&cpu);
            nmi = false;
        }

        // execute instruction
        while( d6502_tick(&cpu) > 0 ) {
        }

        instruction_counter++; // instruction counter
    }
    fclose(log);

    return 0;
}
