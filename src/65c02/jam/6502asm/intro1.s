
.include "jam.inc"
.include "jam_bios.inc"

.define DELAY 200
.define LEDREG $DF04

vector = $10
tmp8   = $12
sinbuf = $cb00
idxbuf = $cac0
copbuf = $caa0

frmcnt = $13

   lda   #$80
   sta   LEDREG
   lda   #$93
   sta   LEDREG

   ldx   #$00
   stx   vector+0
   lda   #$cc
   sta   vector+1
   
:
   clc
   lda   vector+0
   sta   idxbuf+$00,x
   adc   #$20
   sta   vector+0
   lda   vector+1
   sta   idxbuf+$20,x
   adc   #$00
   sta   vector+1
   inx
   cpx   #$20
   bcc   :-

   lda   #$cb
   ldx   #$42
   int   GENSINE
   ldx   #$00
:
   lda   sinbuf,x
   lsr
   lsr
   sta   tmp8
   sec
   lda   sinbuf,x
   sbc   tmp8
   sta   sinbuf,X
   inx
   bne   :-

   jmp   start

xbm:
   lda   #$00
   sta   vector+0
   lda   #$cc
   sta   vector+1

   ldx   #$00
@imgloop:
   lda   sorbus,x
   sta   tmp8

   ldy   #$08
@bitloop:
   lda   #$00
   ror   tmp8
   sbc   #$00
   eor   #$ff
   sta   (vector)
   inc   vector+0
   bne   :+
   inc   vector+1
:
   dey
   bne   @bitloop
   inx
   bne   @imgloop

   stz   LEDREG
   rts

xbmoverlay:
   lda   #$00
   sta   vector+0
   lda   #$cc
   sta   vector+1

   ldx   #$00
@imgloop:
   lda   sorbus,x
   sta   tmp8

   ldy   #$08
@bitloop:
   lda   #$00
   ror   tmp8
   bcc   :+
   lda   #$ff ; white
   sta   (vector)
:
   inc   vector+0
   bne   :+
   inc   vector+1
:
   dey
   bne   @bitloop
   inx
   bne   @imgloop

   stz   LEDREG
   rts

clear:
   ldx   #$00
:
   stz   $cc00,x
   stz   $cd00,x
   stz   $ce00,x
   stz   $cf00,x
   inx
   bne   :-
   rts


coppercalc:
   ldx   #$1f
:
   stz   copbuf,x
   dex
   bpl   :-

   ldx   #$07
:
   stx   tmp8
   ldy   frmcnt
   lda   sinbuf,y
   clc
   adc   tmp8
   tay
   lda   coltab0,x
   ora   copbuf,y
   sta   copbuf,y

   clc
   lda   frmcnt
   adc   #$20
   tay
   lda   sinbuf,y
   clc
   adc   tmp8
   tay
   lda   coltab1,x
   ora   copbuf,y
   sta   copbuf,y

   clc
   lda   frmcnt
   adc   #$40
   tay
   lda   sinbuf,y
   clc
   adc   tmp8
   tay
   lda   coltab2,x
   ora   copbuf,y
   sta   copbuf,y

   clc
   lda   frmcnt
   adc   #$60
   tay
   lda   sinbuf,y
   clc
   adc   tmp8
   tay
   lda   coltab3,x
   ora   copbuf,y
   sta   copbuf,y

   dex
   bpl   :-
   rts

black2copper:
   ldx   #$00
   ldy   #$00
   sty   vector+0
   lda   #$cc
   sta   vector+1
@loop:
   lda   (vector),y
   bne   @noblack
   lda   copbuf,x
   sta   (vector),y
@noblack:
   tya
   inc
   and   #$1f
   bne   :+
   inx
:
   iny
   bne   @loop
   inc   vector+1
   lda   vector+1
   cmp   #$d0
   bne   @loop
;   int   MONITOR
   rts

copperfill:
   ldx   #$00
   ldy   #$00
   sty   vector+0
   lda   #$cc
   sta   vector+1
@loop:
   lda   copbuf,x
   sta   (vector),y
   tya
   inc
   and   #$1f
   bne   :+
   inx
:
   iny
   bne   @loop
   inc   vector+1
   lda   vector+1
   cmp   #$d0
   bne   @loop
;   int   MONITOR
   rts

coplay:
   ldx   #$00
   stx   vector+0
   lda   #$cc
   sta   vector+1

   ldx   #$1f
@loop:
   ldy   #$1f
@line:
   lda   (vector),y
   bne   :+
   lda   copbuf,x
   sta   (vector),y
:
   dey
   bpl   @line

   clc
   lda   vector+0
   adc   #$20
   sta   vector+0
   bcc   :+
   inc   vector+1
:
   dex
   bpl   @loop
delay12:
   rts

start:
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

:
   jsr   CHRIN
   cmp   #$03
   bne   :-

:
   inx
   bne   :-
   iny
   bne   :-

   stz   TMIMRL
   stz   TMIMRH
   jsr   clear
   stz   $df04

   jmp   ($fffc)

irqhandler:
   pha
   phx
   phy

.if 0
   jsr   xbm
   jsr   coppercalc
   jsr   black2copper ;coplay
.else
   jsr   coppercalc
   jsr   copperfill
   jsr   xbmoverlay
.endif

   ply
   plx
   pla
   stz   $df04
   bit   TMIMRL
   inc   frmcnt
   inc   frmcnt
   inc   frmcnt
   inc   frmcnt
   inc   frmcnt
   rti


coltab0:
   .byte %00000001,%00000010,%00000010,%00000011,%00000011,%00000010,%00000010,%00000001
coltab1:
   .byte %00000100,%00001000,%00001000,%00001100,%00001100,%00001000,%00001000,%00000100
coltab2:
   .byte %00010000,%00100000,%00100000,%00110000,%00110000,%00100000,%00100000,%00010000
coltab3:
   .byte %01000000,%10000000,%10000000,%11000000,%11000000,%10000000,%10000000,%01000000

sorbus:
.include "sorbus.inc"
