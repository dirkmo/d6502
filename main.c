#include "d6502.h"
#include "instruction_table.h"
#include <stdio.h>
#include <string.h>

uint8_t memory[0x10000];

int EMULATION_END = 0;
int run_count = 0;

void writebus(uint16_t addr, uint8_t dat) {
    memory[addr] = dat;
}

uint8_t readbus(uint16_t addr) {
    return memory[addr];
}

void load_program(uint16_t addr, const char *fn) {
    FILE *f = fopen(fn, "r");
    int i = addr;
    while(!feof(f) && i < sizeof(memory)) {
        if (fread(memory + i, 1, 1, f) > 0) {
            // printf("%02X ", memory[i]);
            i++;
        } else {
            break;
        }
    }
    //printf("\n");
    fclose(f);
}

void print_regs(d6502_t *cpu) {
    printf("st: %02X (%c%c-%c%c%c%c%c)\n", cpu->st,
        cpu->st & 0x80 ? 'N' : 'n',
        cpu->st & 0x40 ? 'V' : 'v',
        cpu->st & 0x10 ? 'B' : 'b',
        cpu->st & 0x08 ? 'D' : 'd',
        cpu->st & 0x04 ? 'I' : 'i',
        cpu->st & 0x02 ? 'Z' : 'z',
        cpu->st & 0x01 ? 'C' : 'c');
    printf("A: %02X, X: %02X, Y: %02X, SP: %02X, PC: %02X\n", cpu->a, cpu->x, cpu->y, cpu->sp, cpu->pc);
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
    } else if(strstr(cmd, "run") != 0 && sscanf(cmd, "run %d", &addr) == 1) {
        run_count = addr;
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

int main(int argc, char *argv[]) {
    char buf[256];
    load_program(0xc000, "nestest/nestest.bin");
    d6502_t cpu;
    d6502_init(&cpu);
    cpu.read = readbus;
    cpu.write = writebus;
    
    memory[0xFFFC] = 0;
    memory[0xFFFD] = 0xc0;
    d6502_reset(&cpu);

    FILE *log = fopen("log.txt", "w");

    int instruction_counter = 1;
    char asmcode[32];
    char raw[16];
    char logstr[128];
    while( EMULATION_END == 0) {
        d6502_disassemble(&cpu, cpu.pc, asmcode);
        get_raw_instruction(&cpu, raw);
        print_regs(&cpu);
        do {
            printf("\n%d $%04X: %s   %s> ", instruction_counter, cpu.pc, raw, asmcode);
            if( instruction_counter < run_count ) break;
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

        // execute instruction
        while(d6502_tick(&cpu));
        instruction_counter++; // instruction counter
    }
    fclose(log);
    return 0;
}
