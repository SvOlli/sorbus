
.include "../native_bios.inc"

.segment "CODE"
   jsr   PRINT
   .byte 10,"You just loaded a 51k executable into RAM."
   .byte 10,"(Which is the maximum size.)"
   .byte 10,10,0
   jmp   ($fffc)        ; a reset is the best way to end

   .res  $cbaf, $00     ; rough guess for size to pad to 51k ($0400-$D000)
