
.include "jam.inc"

.define DELAY 400

start:
   lda   #$80
   sta   $df04
   lda   #$93
   sta   $df04

   sei
   lda   #<irqhandler
   sta   UVNBI+0
   lda   #>irqhandler
   sta   UVNBI+1

   lda   #<DELAY
   sta   TMIMRL
   lda   #>DELAY
   sta   TMIMRH
   cli

loop:
   lda   RANDOM
   sta   $10
   lda   RANDOM
   and   #$03
   ora   #$cc
   sta   $11
   lda   RANDOM
   sta   ($10)
   jmp   loop

irqhandler:
   bit   TMIMRL
   stz   $df04
   rti
