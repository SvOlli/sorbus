
.include "jam.inc"
.include "jam_bios.inc"

; a test program for internal drive access

start:
   jsr   PRINT
   .byte 10,"should end with error code $F2 (illegal sector)"
   .byte 10,"reading all sectors: $",0
   stz   ID_LBA+0
   stz   ID_LBA+1
   lda   #$03
   sta   ID_MEM+1
@loop:
   stz   ID_MEM+0
   lda   ID_LBA+0
   ldx   ID_LBA+1
   int   PRHEX16
   jsr   PRINT
   .byte 8,8,8,8,0
   stz   IDREAD
:
   lda   IDREAD
   bpl   :-
   cmp   #$f0
   bcc   @loop

   pha
   jsr   PRINT
   .byte 10,"error code: $",0
   pla
   int   PRHEX8

   lda   #$0a
   jsr   CHROUT

   jmp   ($fffc)

