\ This file is licensed under the terms of the 2-clause BSD license. Please
\ see the COPYING file in the root project directory for the full text.

.include "cpm65.inc"
.include "drivers.inc"

.expand 1

BANK = $DF00
RESET = $FFFC

\ --- Resident part starts at the top of the file ---------------------------

.zproc start
    lda #<message
    ldx #>message
    ldy #BDOS_PRINTSTRING
    jsr BDOS

    lda #$01
    sta BANK
    jmp (RESET)
    
message:
    .byte "Rebooting into Sorbus kernel", 13, 10, 0

\ vim: sw=4 ts=4 et

