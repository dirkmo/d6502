#include "apu.h"

static uint8_t joy1 = 0;
static uint8_t joy2 = 0;
static uint8_t strobe = 0;

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
            val = joy1 & 1;
            if (strobe == 0) {
                joy1 = joy1 >> 1;
            }
            break;
        case 0x17: // Joypad #2
            val = joy2 & 1;
            if (strobe == 0) {
                joy2 = joy2 >> 1;
            }
            break;
        default: ;
    }
    return val;
}
