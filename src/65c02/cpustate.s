
.segment "CODE"
_A:
   .byte $00
_X:
   .byte $00
_Y:
   .byte $00
_SP:
   .byte $00
_P:
   .byte $00

; labels are all "<", to make sure that zeropage addressing is used
start:
   sta   <_A
   stx   <_X
   sty   <_Y
   tsx
   stx   <_SP
   ldx   <_X
   php
   pla
   sta   <_P
   lda   <_A
   rti

loop:
   jmp   loop

;.segment "VECTORS"
nmi:
   .word start
reset:
   .word loop
irq:
   .word loop

