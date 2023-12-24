\ This file is licensed under the terms of the 2-clause BSD license. Please
\ see the COPYING file in the root project directory for the full text.

.include "cpm65.inc"

.expand 1

BANK = $DF00
TRAP = $DF01
RESET = $FFFC

\ --- Resident part starts at the top of the file ---------------------------

.zproc start
    \ misusing cpm_fcb as first argument to parse
    lda cpm_fcb+1
    cmp #'E'
    beq reboot
    cmp #'T'
    beq trap

    lda #<m_help
    ldx #>m_help
    ldy #BDOS_PRINTSTRING
    jmp BDOS

reboot:
    lda #<m_reboot
    ldx #>m_reboot
    ldy #BDOS_PRINTSTRING
    jsr BDOS

    lda #$01
    sta BANK
    jmp (RESET)

trap:
    sta TRAP
    rts

m_help:
    .byte "Parameters: exit trap", 10, 0
m_reboot:
    .byte "Rebooting into Sorbus kernel", 10, 0

\ vim: sw=4 ts=4 et

