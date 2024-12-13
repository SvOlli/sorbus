
.include "jam.inc"
.include "jam_bios.inc"

; subroutines to count and evaluate processor cycles

.export savebuffer, diffbuffer, printbuffer

savebuffer:
   ldx   #$00         ; start save at least significant byte
:
   lda   CYCNTR,x
   sta   BUFFER,x
   inx                ; must be incremented, base address copies to shadow
   cpx   #$03
   bcc   :-
   rts

diffbuffer:
   ldx   #$00         ; start calc at least significant byte
   sec
   php
:
   plp                ; get carry from stack from previous loop
   lda   BUFFER,x
   sbc   CYCNTR,x
   sta   BUFFER,x
   php                ; save carry on stack for next loop
   inx                ; must be incremented, base address copies to shadow
   cpx   #$03
   bcc   :-
   plp
   rts

printbuffer:
   ldx   #$03         ; start print at most significant byte
:
   lda   CYCNTR,x
   int   PRHEX8
   dex
   bpl   :-
   rts

