.SETCPU "6502"

.code
reset:  cli
        ;sei

loop:
        LDA #$01
        STA $0
        jmp loop

nmi:    lda #$02
        sta $0
        jmp nmi

intr:   lda #$03
        sta $0
        jmp intr
