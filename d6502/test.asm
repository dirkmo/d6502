.SETCPU "6502"

.code
reset:  cli
        ;sei
        ldx #0
        ldy #0

loop:
        LDA #$01
        STA $0
        jmp loop

nmi:    inx
        rti

intr:   iny
        rti
