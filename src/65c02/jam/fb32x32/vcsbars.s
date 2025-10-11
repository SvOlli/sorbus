
.include "fb32x32.inc"

.define     FRAMEBUFFER $cc00
.define     DELAY_IRQ   500

sin_base       := $c000
sin_x          := $c100
sin_y          := $c200

frmcnt         := $10

count          := $40
xstart         := $42
ystart         := $43
xadd           := $44
yadd           := $45
vector         := $4a

start:
   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$01
   int   FB32X32

   lda   #$10
   ldx   #>sin_base
   int   GENSINE

   stz   xstart
   lda   #$b7
   sta   ystart

   sei
   lda   #<irqhandler
   sta   UVNBI+0
   lda   #>irqhandler
   sta   UVNBI+1
   lda   #<DELAY_IRQ
   sta   TMIMRL
   lda   #>DELAY_IRQ
   sta   TMIMRH
   cli

   jsr   PRINT
   .byte 10,"(CTRL-C to quit) ",0

@mainloop:
   jsr   CHRIN
   cmp   #$03
   bne   @mainloop

   sei
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

   inc   frmcnt

   lda   #FB32X32_CMAP_A800_RORD
   sta   FB32X32_COLMAP

   stz   vector+0
   lda   #>FRAMEBUFFER
   sta   vector+1

   lda   #$01
   sta   xadd
   lda   #$fd
   sta   yadd

   ldx   xstart
   ldy   ystart

   lda   #$20
   sta   count
@loop:
   clc
   lda   sin_base,x
   adc   sin_base,y
   ror
   sta   (vector)
   inc   vector+0
   bne   :+
   inc   vector+1
:
   txa
   clc
   adc   xadd
   tax

   tya
   clc
   adc   yadd
   tay

   dec   count
   bne   @loop

   inc   xstart
   inc   ystart

   stz   FB32X32_SRC+0
   lda   #>FRAMEBUFFER
   sta   FB32X32_SRC+1

   ldx   #$1f
   stz   FB32X32_DEST_Y

   lda   #$1f
   stz   FB32X32_WIDTH
   sta   FB32X32_HEIGHT
   stz   FB32X32_STEP
:
   stx   FB32X32_DEST_X

   stz   FB32X32_COPYN

   dex
   bpl   :-

   stz   FB32X32_COPY

   bit   TMIMRL         ; acknoledge timer
   ply
   plx
   pla
   rti
