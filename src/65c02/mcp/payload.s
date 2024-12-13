
.segment "CODE"

BUFFER := $1000

   jmp   example1
   jmp   example2
   jmp   example3
   jmp   example4
   jmp   example5

example1:
   clc
   cld
   ldx   #$01
   stx   BUFFER+1
   dex
   stx   BUFFER

@loop:
   lda   BUFFER,x
   inx
   adc   BUFFER,x
   bcs   *
   inx
   sta   BUFFER,x
   dex
   bne   @loop

example2:
   jsr   jsrtest
   jmp   *

example3:
   lda   #$42
   pha
   pla
   jmp   *

example4:
   lda   #<irqtest
   sta   $fffe
   lda   #>irqtest
   sta   $ffff
   nop
   nop
   nop
   cli
   jmp   *

example5:
   lda   #<irqtest
   sta   $fffe
   lda   #>irqtest
   sta   $ffff
   brk
   nop
   nop
   nop
   jmp   *

jsrtest:
   nop
   nop
   rts
   brk

irqtest:
   pha
   nop
   nop
   pla
   rti
   brk
