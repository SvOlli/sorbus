
.include "fb32x32.inc"

.define DELAY 500

vector = $10
tmp8   = $12
frmcnt = $13
chroma = $14
luma   = $15

vector1 = $20
vector2 = $22

tmpbuf      := $0200
FRAMEBUFFER := $cc00

rtab     = $02cd
gtab     = $02de
btab     = $02ef

start:
   jmp   init

setupfb:
   lda   #<FRAMEBUFFER
   sta   vector1+0
   lda   #<(FRAMEBUFFER+$1F)
   sta   vector2+0
   lda   #>FRAMEBUFFER
   sta   vector1+1
   sta   vector2+1

   ldy   #$00
@loop2:
   tya
   and   #$0f
   sta   tmp8
   tya
   lsr
   lsr
   lsr
   lsr
   cmp   tmp8
   bcc   :+
   lda   tmp8
:
   sta   (vector1)
   sta   (vector2)

   inc   vector1+0
   dec   vector2+0
   iny
   tya
   and   #$0f
   bne   @loop2

   lda   vector2+0
   clc
   adc   #$30
   sta   vector2+0
   lda   vector1+0
   clc
   adc   #$10
   sta   vector1+0
   lda   vector1+1
   adc   #$00
   sta   vector1+1
   sta   vector2+1
   cmp   #>(FRAMEBUFFER+$200)
   bcc   @loop2

   ldx   #$00
   ldy   #$ff
@loop3:
   lda   FRAMEBUFFER+$000,x
   sta   FRAMEBUFFER+$300,y
   lda   FRAMEBUFFER+$100,x
   sta   FRAMEBUFFER+$200,y
   dey
   inx
   bne   @loop3
   rts

colupdate:
   lda   #$01
   sta   FB32X32_CCOLMAP_IDX

   ldx   #$0f
:
   lda   rtab+1,x
   sta   FB32X32_CCOLMAP_R
   lda   rtab+0,x
   sta   rtab+1,x
   lda   gtab+1,x
   sta   FB32X32_CCOLMAP_G
   lda   gtab+0,x
   sta   gtab+1,x
   lda   btab+1,x
   sta   FB32X32_CCOLMAP_B
   lda   btab+0,x
   sta   btab+1,x
   dex
   bpl   :-
   
   lda   luma
   dec
   and   #$0f
   sta   luma
   sta   rtab
   sta   gtab
   sta   btab
   bne   :++

   lda   chroma
:
   inc
   and   #$07
   beq   :-
   sta   chroma
:

   lda   chroma
   lsr
   bcs   :+
   stz   rtab
:
   lsr
   bcs   :+
   stz   gtab
:
   lsr
   bcs   :+
   stz   btab
:
   lda   #$01
   sta   FB32X32_COLMAP ; required to activate new palette

   lda   tmp8
   beq   :+

   lda   RANDOM
   and   #$0f
   sta   rtab

   lda   RANDOM
   and   #$0f
   sta   gtab

   lda   RANDOM
   and   #$0f
   sta   btab
:

   rts

irqhandler:
   pha
   phx
   phy

   jsr   colupdate
   stz   FB32X32_COPY

   lda   TMIMRL         ; acknoledge timer
   ply
   plx
   pla
   rti

init:
   lda   #<FRAMEBUFFER
   sta   vector+0
   ldx   #>FRAMEBUFFER
   stx   vector+1
   ldy   #$00
   int   FB32X32
   lda   #FB32X32_CMAP_C64
   sta   FB32X32_COLMAP

   lda   #$07
   sta   chroma
   stz   luma

   jsr   setupfb

   sei
   lda   #<irqhandler
   sta   UVNBI+0
   lda   #>irqhandler
   sta   UVNBI+1
   lda   #<DELAY
   sta   TMIMRL
   lda   #>DELAY
   sta   TMIMRH

   lda   #$01 ; FB32X32_CMAP_C64
   sta   FB32X32_COLMAP
   sta   chroma
   sta   luma

   cli
   stz   tmp8

   jsr   PRINT
   .byte 10,"press space to toggle mode"
   .byte 10,"(CTRL-C to quit) ",0

@mainloop:
   jsr   CHRIN
   cmp   #$20
   bne   :+
   lda   tmp8
   eor   #$01
   sta   tmp8
:
   cmp   #$03
   bne   @mainloop

   stz   TMIMRL
   stz   TMIMRH

   jmp   ($fffc)

