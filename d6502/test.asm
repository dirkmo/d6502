.SETCPU "6502"

.code

reset:
        LDA #$01
        STA $0
        jmp reset

nmi:    lda #$02
        sta $0
        jmp nmi

intr:   lda #$03
        sta $0
        jmp intr
