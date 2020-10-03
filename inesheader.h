#ifndef _INESHEADER_H
#define _INESHEADER_H

#include <stdint.h>

/// http://wiki.nesdev.com/w/index.php/INES

// An iNES file consists of the following sections, in order:

//     Header (16 bytes)
//     Trainer, if present (0 or 512 bytes)
//     PRG ROM data (16384 * x bytes)
//     CHR ROM data, if present (8192 * y bytes)
//     PlayChoice INST-ROM, if present (0 or 8192 bytes)
//     PlayChoice PROM, if present (16 bytes Data, 16 bytes CounterOut) (this is often missing, see PC10 ROM-Images for details)

// Some ROM-Images additionally contain a 128-byte (or sometimes 127-byte) title at the end of the file.

// The format of the header is as follows:

//     0-3: Constant $4E $45 $53 $1A ("NES" followed by MS-DOS end-of-file)
//     4: Size of PRG ROM in 16 KB units
//     5: Size of CHR ROM in 8 KB units (Value 0 means the board uses CHR RAM)
//     6: Flags 6 - Mapper, mirroring, battery, trainer
//     7: Flags 7 - Mapper, VS/Playchoice, NES 2.0
//     8: Flags 8 - PRG-RAM size (rarely used extension)
//     9: Flags 9 - TV system (rarely used extension)
//     10: Flags 10 - TV system, PRG-RAM presence (unofficial, rarely used extension)
//     11-15: Unused padding (should be filled with zero, but some rippers put their name across bytes 7-15)

typedef struct {
    char magic[4];
    // byte 4
    uint8_t nPRGROM16k;
    // byte 5
    uint8_t nCHRROM8k;
    // byte 6
    unsigned Vh : 1; // 1:vertical, 0: horizontal
    unsigned bat : 1;
    unsigned trainer : 1;
    unsigned four : 1; // 1: ignore Vh, four screen
    unsigned mapperlo : 4;
    // byte 7
    unsigned vs : 1;
    unsigned res1 : 3;
    unsigned mapperhi : 4; 
    // byte 8
    uint8_t nRAMbanks8k;
    uint8_t pal;
    uint8_t reserved[6];
} inesheader_t;

// possible trainer data at 16 .. 527

// VROMs start at 528

#endif