
.include "fb32x32.inc"

.define DELAY 400

start:
   lda   #$00
   ldx   #$cc
   ldy   #$01
   int   FB32X32

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
   stz   FB32X32_COPY
   rti
