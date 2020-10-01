#include "apu.h"

static uint8_t joy1 = 0;
static uint8_t joy2 = 0;
static uint8_t strobe = 0;
static uint8_t joy1shift = 0;
static uint8_t joy2shift = 0;

void apu_report_buttonpress(button_t button, bool pressed) {
    if (pressed) {
        joy1 |= button;
    } else {
        joy1 &= ~button;
    }
}

void apu_write(uint8_t addr, uint8_t dat) {
    switch (addr) {
        case 0x14: // OAMDMA
            break;
        case 0x15:
            break;
        case 0x16: // Joypad #1
            strobe = dat & 1;
            if (strobe == 1) {
                joy1shift = joy1;
                joy2shift = joy2;
            }
            break;
        case 0x17: // Joypad #2
            break;
        default: ;
    }
}

uint8_t apu_read(uint8_t addr) {
    uint8_t val = 0;
    switch(addr) {
        case 0x14: // OAMDMA
            break;
        case 0x15:
            break;
        case 0x16:// Joypad #1
            val = joy1shift & 1;
            if (strobe == 0) {
                joy1shift = joy1shift >> 1;
            }
            break;
        case 0x17: // Joypad #2
            val = joy2shift & 1;
            if (strobe == 0) {
                joy2shift = joy2shift >> 1;
            }
            break;
        default: ;
    }
    return val;
}
