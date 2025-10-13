
.include "fb32x32.inc"

.define     FRAMEBUFFER $cc00
.define     DELAY_IRQ   500

sin_base       := $c000
vvalues        := $c100
hvalues        := $c120

frmcnt         := $10

count          := $40
vxstart        := $42
vystart        := $43
vxadd          := $44
vyadd          := $45
hxstart        := $46
hystart        := $47
hxadd          := $48
hyadd          := $49
vector         := $4a

start:
   lda   #$01
   sta   FB32X32_CCOLMAP_IDX

   ldx   #$00
:
   txa
   lsr
   lsr
   lsr
   lsr
   sta   FB32X32_CCOLMAP_R
   txa
   and   #$0f
   sta   FB32X32_CCOLMAP_G
   stz   FB32X32_CCOLMAP_B
   inx
   bne   :-

   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$01
   int   FB32X32
   sty   FB32X32_COLMAP

   lda   #$01
   ldx   #>sin_base
   int   GENSINE

   lda   #$03
   sta   vxadd
   lda   #$f9
   sta   vyadd

   stz   vxstart
   lda   #$b7
   sta   vystart

   lda   #$09
   sta   hxadd
   lda   #$f3
   sta   hyadd

   stz   hxstart
   lda   #$97
   sta   hystart

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

   jsr   vertical

   stz   vector+0
   lda   #>FRAMEBUFFER
   sta   vector+1

   ldx   #$1f
@loop1:
   ldy   #$1f
@loop2:
   lda   vvalues,x
   asl
   asl
   asl
   asl
   ora   hvalues,y
   sta   (vector)
   inc   vector+0
   bne   :+
   inc   vector+1
:
   dey
   bpl   @loop2
   dex
   bpl   @loop1

   stz   FB32X32_COPY

   bit   TMIMRL         ; acknoledge timer
   ply
   plx
   pla
   rti

vertical:
   stz   vector+0
   lda   #>vvalues
   sta   vector+1

   ldx   vxstart
   ldy   vystart

   lda   #$1f
   sta   count
@vloop:
   clc
   lda   sin_base,x
   lsr
   adc   sin_base,y
   ror
   sta   (vector)
   inc   vector+0
   txa
   clc
   adc   vxadd
   tax

   tya
   clc
   adc   vyadd
   tay

   dec   count
   bpl   @vloop

   inc   vxstart
   dec   vystart
   dec   vystart

   ldx   hxstart
   ldy   hystart

   lda   #$1f
   sta   count
@hloop:
   clc
   lda   sin_base,x
   lsr
   adc   sin_base,y
   ror
   sta   (vector)
   inc   vector+0
   txa
   clc
   adc   hxadd
   tax

   tya
   clc
   adc   hyadd
   tay

   dec   count
   bpl   @hloop

   dec   hxstart
   inc   hystart
   inc   hystart

   rts
