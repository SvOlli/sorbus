
.segment "CODE"

   ; should start at $0000
   ; whole memory is just $20 (=32) bytes
start:
   cld
   clc
   .byte $5c
   .word is816
   sec
   bcc   isce02
   lda   #$ea
   .byte $eb
   nop
   bne   isc02
is02:
   lda   #$01
   .byte $2c
isc02:
   lda   #$02
   .byte $2c
is816:
   lda   #$03
   .byte $2c
isce02:
   lda   #$04
   sta   $ff

   jmp   start
   .byte $00,$ff
