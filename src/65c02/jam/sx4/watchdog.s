
.include "jam_bios.inc"
.include "jam.inc"

start:
   lda   #$08           ; 2048ms
   stz   WDCL
   sta   WDCM
   stz   WDCH
   int   MONITOR        ; wait in monitor for watchdog to occur
   jmp   ($fffc)        ; reset when leaving monitor
