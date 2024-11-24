
.include "../native_bios.inc"
.include "../native.inc"

.segment "CODE"
   ldx   #$00
   stx   TMP16+0
   lda   #$10
   sta   TMP16+1

   ldy   #$00
:
   txa
   lsr
   lsr
   lsr
   lsr
   sta   ASAVE
   txa
   asl
   asl
   asl
   asl
   ora   ASAVE
   jsr   writebyte
   lda   #$ea
   jsr   writebyte
   jsr   writebyte
   jsr   writebyte
   inx
   bne   :-

   jsr   PRINT
   .byte 10,"all opcodes have been written to $1000-$13ff",0

   jmp   ($fffc)

writebyte:
   sta   (TMP16),y
   iny
   bne   :+
   inc   TMP16+1
:
   rts
