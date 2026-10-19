
.include "jam_bios.inc"
.include "jam.inc"

start:
   jsr   PRINT
   .byte "Triggering watchdog in ~2 seconds.\n"
   .byte "Dropping to monitor for now.\n\n", 0
   lda   #$08           ; 2048ms
   stz   WDCL
   sta   WDCM
   stz   WDCH
   stz   WDRUN
   int   MONITOR        ; wait in monitor for watchdog to occur
   jmp   ($fffc)        ; reset when leaving monitor
