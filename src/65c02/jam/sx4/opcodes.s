
.include "jam_bios.inc"
.include "jam.inc"

; this program will write all opcodes from $00 to $ff to memory
; followed by $EA/nop, so a clean disassembly is possible
; intended to generate normalized disassemblies from
; System Monitor

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
   .byte 10,"all opcodes have been written to $1000-$13ff"
   .byte 10,"now run ",$22,"d 1000 1400",$22," to run test",0

   int   MONITOR
   jmp   ($fffc)

writebyte:
   sta   (TMP16),y
   iny
   bne   :+
   inc   TMP16+1
:
   rts
