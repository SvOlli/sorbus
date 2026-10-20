
.include "jam_bios.inc"
.include "jam.inc"

start:
   jsr   PRINT
   .byte "Triggering watchdog in ~2 seconds.",10
   .byte "You can enter 'g' to re-trigger.",10
   .byte "Dropping to monitor for now.",10,10,0
   lda   #$08           ; 2048ms
   stz   WDCL
   sta   WDCM
   stz   WDCH
:
   stz   WDRUN
   int   MONITOR        ; wait in monitor for watchdog to occur
   bra   :-
