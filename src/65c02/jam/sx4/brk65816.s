
.include "jam_bios.inc"
.include "jam.inc"

.define NV65816COP   $ffe4
.define NV65816BRK   $ffe6
.define NV65816ABORT $ffe8
.define NV65816NMI   $ffea
.define NV65816IRQ   $ffee
.define EV65816COP   $fff4
.define EV65816ABORT $fff8
.define EV65816NMI   $fffa
.define EV65816RESET $fffc
.define EV65816IRQ   $fffe

; override default cpu
.setcpu "65816"
.smart

start:
   int   COPYBIOS
   stz   BANK

   sei
   clc
   xce

   rep   #$20
   lda   #payload
   sta   NV65816BRK
   sep   #$30
   php
   int   $08
   plp
   rep   #$30
   int   $10

   jmp   ($fffc)

payload:
   php
   sep   #$30
   stz   TRAP
   plp
   rti
