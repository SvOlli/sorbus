
.include "../native_bios.inc"
.include "../native.inc"

; A small program that implements printing a maze-like structure.
; Implementations in C and BASIC exists also.

.define  TABLE $0200

.segment "CODE"
   ldx   #$00
   ldy   #$00
:
   lda   labydat,y
   sta   TABLE,x
   iny
   cpy   #$0b
   bcc   :+
   ldy   #$00
:
   inx
   bne   :--

:
   lda   #226
   jsr   CHROUT
   lda   #148
   jsr   CHROUT
   ldx   RANDOM
   lda   TABLE,x
   jsr   CHROUT
   jsr   CHRIN
   cmp   #$03
   bne   :-
   jmp   ($FFFC)

labydat:
   .byte 128,130,140,144,148,152,156,164,172,180,188
