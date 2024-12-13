
.include "jam_bios.inc"
.include "jam.inc"

; short program that will allow to follow the flow of the
; user BRK vector to determine behaviour details and speed

start:
   stz   BANK
   lda   UVBRK+0
   pha
   lda   UVBRK+1
   pha
   lda   #<payload
   sta   UVBRK+0
   lda   #>payload
   sta   UVBRK+1

   lda   #'S'
   ldx   #'B'
   ldy   #'C'
   clc

   php
   int   INTUSER
   php

   sta   MON_A
   stx   MON_X
   sty   MON_Y

   pla
   pla

   lda   BANK
   sta   TRAP

   pla
   sta   UVBRK+1
   pla
   sta   UVBRK+0

   int   MONITOR

   jmp   ($fffc)

payload:
   ora   #$80
   rts
