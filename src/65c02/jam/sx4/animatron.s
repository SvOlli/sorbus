
.include "jam_bios.inc"
.include "jam.inc"

start:
   sei

   jsr   PRINT
   .byte 10,"loading song: ",0

   ldx   #$00
:
   lda   filename,x
   beq   :+
   jsr   CHROUT
   inx
   bne   :-
:

   jsr   PRINT
   .byte 10,"memory: ",0

   lda   #<filename
   ldx   #>filename
   ldy   #$01           ; user 1
   int   CPMNAME

   lda   #$7a
   sta   CPM_SADDR+0
   lda   #$0f
   sta   CPM_SADDR+1
   int   CPMLOAD

   lda   CPM_SADDR+0
   ldx   CPM_SADDR+1
   int   PRHEX16
   lda   #'-'
   jsr   CHROUT
   lda   CPM_EADDR+0
   ldx   CPM_EADDR+1
   int   PRHEX16

   jsr   PRINT
   .byte 10,"setting up IRQ",10,0

   lda   #$00
   jsr   $1000

   lda   #<irqhandler
   sta   UVNBI+0
   lda   #>irqhandler
   sta   UVNBI+1

   lda   #200/4           ; 20.0ms = 50 times per second  , /4 for quad-speed-player
   sta   TMIMRL
   stz   TMIMRH

   cli

   jsr   PRINT
   .byte "press ctrl-C to stop",10,0

:
   jsr   CHRIN
   cmp   #$03
   bne   :-
   stz   TMIMRL
   stz   TMIMRH
   jmp   ($FFFC)        ; reset

filename:
   .byte "animatron.sid",0

irqhandler:
   pha
   phx
   phy

   lda   TMIMRL         ; acknoledge timer
   jsr   $1003          ; play frame

   ply
   plx
   pla
   rti
