
.include "fb32x32.inc"

.define DELAY 500
.define LEDREG $DF04

vector = $10
tmp8   = $12
sinbuf = $cb00
idxbuf = $cac0
copbuf = $caa0
FRAMEBUFFER := $cc00

frmcnt = $13

   sei
   lda   #<FRAMEBUFFER
   sta   vector+0
   ldx   #>FRAMEBUFFER
   stx   vector+1
   ldy   #$00
   int   FB32X32

   ;ldy   #$00
:
   clc
   lda   vector+0
   sta   idxbuf+$00,y
   adc   #$20
   sta   vector+0
   lda   vector+1
   sta   idxbuf+$20,y
   adc   #$00
   sta   vector+1
   iny
   cpy   #$20
   bcc   :-

   lda   #>sinbuf
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

   jsr   xbm

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
   jsr   xbm
   stz   FB32X32_COPY

   jmp   ($fffc)


xbm:
   lda   #<FRAMEBUFFER
   sta   vector+0
   lda   #>FRAMEBUFFER
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


copperfill:
   stz   FB32X32_WIDTH
   stz   FB32X32_STEP
   stz   FB32X32_DEST_Y
   lda   #<copbuf
   sta   FB32X32_SRC+0
   lda   #>copbuf
   sta   FB32X32_SRC+1
   ldx   #$00
:
   stx   FB32X32_DEST_X
   stz   FB32X32_COPYN
   inx
   cpx   #$20
   bne   :-
   rts


xbmoverlay:
   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$00
   int   FB32X32

   lda   #$01              ; transparent color is $00 by default
   sta   FB32X32_COPY
   rts


irqhandler:
   pha
   phx
   phy

   jsr   coppercalc
   jsr   copperfill
   jsr   xbmoverlay

   bit   TMIMRL
   inc   frmcnt
   inc   frmcnt
   inc   frmcnt

   ply
   plx
   pla
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
