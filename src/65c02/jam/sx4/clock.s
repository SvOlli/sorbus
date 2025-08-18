
.include "jam_bios.inc"
.include "jam.inc"

inputbuffer = $0280
time_h := $08
time_m := $09
time_s := $0a

start:
   sei
   lda   #<irqhandler
   sta   UVNBI+0
   lda   #>irqhandler
   sta   UVNBI+1
loop:
   jsr   PRINT
   .byte 10,"Input time (HHMMSS): ",0
   lda   #<inputbuffer
   ldx   #>inputbuffer
   ldy   #$06
   int   LINEINPUT
   ldx   #$05
:
   lda   inputbuffer,x
   cmp   #'0'
   bcc   loop
   cmp   #'9'+1
   bcs   loop
   dex
   bpl   :-

   ldy   #$00
   ldx   #$00
:
   lda   inputbuffer,y
   iny
   asl
   asl
   asl
   asl
   sta   time_h,x
   lda   inputbuffer,y
   iny
   and   #$0f
   ora   time_h,x
   sta   time_h,x
   inx
   cpx   #$03
   bcc   :-

   lda   #$0a
   jsr   CHROUT
   lda   #<10000
   sta   TMIMRL
   lda   #>10000
   sta   TMIMRH
   lda   inputbuffer
   cli
:
   jsr   CHRIN
   cmp   #$03
   bne   :-
   stz   TMIMRL
   stz   TMIMRH
   jmp   ($FFFC)


irqhandler:
   pha
   phx
   sed
   sec                  ; add a second

   lda   time_s
   adc   #$00
   cmp   #$60           ; sec carry for 60 seconds or more
   bcc   :+
   lda   #$00           ; if overflow, when clear
:
   sta   time_s

   lda   time_m
   adc   #$00
   cmp   #$60           ; sec carry for 60 minutes or more
   bcc   :+
   lda   #$00
:
   sta   time_m

   lda   time_h
   adc   #$00
   cmp   #$24
   bcc   :+
   lda   #$00
:
   sta   time_h

   cld

   lda   #$0d
   jsr   CHROUT

   ldx   #$00
:
   lda   time_h,x
   int   PRHEX8
   lda   septab,x
   jsr   CHROUT
   inx
   cpx   #$03
   bcc   :-
   plx
   lda   TMIMRL
   pla
   rti

septab:
   .byte ":: "
