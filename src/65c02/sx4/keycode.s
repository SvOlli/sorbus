
.include "../native_bios.inc"

main:
   jsr   CHRIN
   bcs   main
   cmp   #$03
   bne   :+
   jmp   ($FFFC)
:
   int   PRHEX8
   lda   #' '
   jsr   CHROUT
   bra   main

