
.include "jam_bios.inc"

; A very small program that just prints out every key received
; as hex values. Sequences are printed as individual keys

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

