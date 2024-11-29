
.include "../native_bios.inc"
.include "../native.inc"

.export     blockrw
.export     iofailed

.import     getaddr
.import     skipspace
.import     prterr
.import     newenter
.import     uppercase

.importzp   FORMAT

error:
   jmp   prterr
blockrw:
   jsr   skipspace
   jsr   uppercase
   cmp   #'R'
   beq   :+
   cmp   #'W'
   beq   :+
   dex
   jmp   prterr
:
   sta   FORMAT
   inx

   jsr   getaddr
   bcs   error
   sta   ID_LBA+0
   sty   ID_LBA+1

   jsr   getaddr
   bcs   error
   sta   ID_MEM+0
   sty   ID_MEM+1

   lda   FORMAT
   cmp   #'W'
   lda   #$00
   rol                  ; Y=0, unless A>='W' -> Y=1
   tay
   sta   IDREAD,y       ; with Y=1 it's IDWRIT
   lda   IDREAD,y
   bne   :+             ; i/o error occured
   rts
:
   jsr   PRINT
   .byte 10,"block ",0
   tya
   bne   :+
   jsr   PRINT
   .byte "read",0
   beq   iofailed
:
   jsr   PRINT
   .byte "write",0

iofailed:
   pla
   pla
   jsr   PRINT
   .byte " failed",0
   jmp   newenter       ; return to newenter instead of loop
