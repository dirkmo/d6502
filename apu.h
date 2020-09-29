#ifndef _APU_H
#define _APU_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BUTTON_A      = 0x01,
    BUTTON_B      = 0x02,
    BUTTON_SELECT = 0x04,
    BUTTON_START  = 0x08,
    BUTTON_UP     = 0x10,
    BUTTON_DOWN   = 0x20,
    BUTTON_LEFT   = 0x40,
    BUTTON_RIGHT  = 0x80,
} button_t;

void apu_report_buttonpress(button_t button, bool pressed);

uint8_t apu_read(uint8_t addr);
void apu_write(uint8_t addr, uint8_t dat);

#endif
