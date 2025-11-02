
.include "fb32x32.inc"

.define     IMPROVED 1

FRAMEBUFFER := $cc00
DELAY       := 500

tmp8        := $10
xp          := $11
yp          := $12
mp          := $13


.segment "CODE"

start:
   jsr   kaleido_init

   jsr   PRINT
   .byte 10,"(CTRL-C to quit) ",10,0

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

@mainloop:
   jsr   CHRIN
   cmp   #$03
   bne   @mainloop

   stz   TMIMRL
   stz   TMIMRH

   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$01
   int   FB32X32

   jmp   ($fffc)


irqhandler:
   pha
   phx
   phy

   jsr   kaleido_run
   lda   TMIMRL         ; acknoledge timer

   ply
   plx
   pla
   rti


kaleido_init:
   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$01
   int   FB32X32

   lda   #FB32X32_CMAP_VDC
   sta   FB32X32_COLMAP

   sta   xp
   sta   yp
   sta   mp
   rts


kaleido_run:
   ldx   #$00

@loop:
   ; yp += (xp>>2) & mp
   lda   xp
   lsr
   lsr
   and   mp
   clc
   adc   yp
   sta   yp

   ; xp -= (yp>>2) & mp
   lda   yp
   lsr
   lsr
   and   mp
   eor   #$ff
   sec
   adc   xp
   sta   xp

   ;lda   xp
   lsr
   lsr
   lsr
   lsr
   sta   tmp8
   lda   yp
   and   #$f0
   ora   tmp8
   tay
.if IMPROVED
   eor   #$0f
   pha
.endif

   txa
   and   #$01
   beq   :+
   txa
   lsr
:
   sta   FRAMEBUFFER,y
.if IMPROVED
   ply
   sta   FRAMEBUFFER+$100,y
.endif

   inx
   cpx   #$10
   bcc   @loop

   inc   xp
   inc   yp
   inc   mp
   jsr   flip16to32
   jmp   show4fb


flip16to32:
   ldx   #$00
.if IMPROVED
   ldy   #$ff
:
   lda   FRAMEBUFFER+$000,x
   sta   FRAMEBUFFER+$300,y
   lda   FRAMEBUFFER+$100,x
   sta   FRAMEBUFFER+$200,y
   dey
   inx
   bne   :-
.else
@loop:
   txa
   eor   #$0f
   tay
   lda   FRAMEBUFFER,x
   sta   FRAMEBUFFER+$100,y

   txa
   eor   #$f0
   tay
   lda   FRAMEBUFFER,x
   sta   FRAMEBUFFER+$200,y

   txa
   eor   #$ff
   tay
   lda   FRAMEBUFFER,x
   sta   FRAMEBUFFER+$300,y
   inx
   bne   @loop
.endif
   rts

show4fb:
   ldy   #$10
   stz   FB32X32_SRC+0
   lda   #>FRAMEBUFFER
   sta   FB32X32_SRC+1
   stz   FB32X32_DEST_X
   stz   FB32X32_DEST_Y
   ldx   #$0f
   stx   FB32X32_WIDTH
   stx   FB32X32_HEIGHT
   stx   FB32X32_STEP
   stz   FB32X32_COPYN

   inc
   sta   FB32X32_SRC+1
   sty   FB32X32_DEST_X
   stz   FB32X32_DEST_Y
   stz   FB32X32_COPYN

   inc
   sta   FB32X32_SRC+1
   stz   FB32X32_DEST_X
   sty   FB32X32_DEST_Y
   stz   FB32X32_COPYN

   inc
   sta   FB32X32_SRC+1
   sty   FB32X32_DEST_X
   sty   FB32X32_DEST_Y
   stz   FB32X32_COPY

   rts
