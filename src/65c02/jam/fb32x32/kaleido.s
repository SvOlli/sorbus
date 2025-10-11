
.include "fb32x32.inc"

FRAMEBUFFER := $cc00

tmp8     := $10
xp       := $11
yp       := $12
mp       := $13

myschedule:


.segment "CODE"

start:
kaleidoscope:
   lda   #FB32X32_CMAP_VDC
   sta   FB32X32_COLMAP

   sta   xp
   sta   yp
   sta   mp

   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$01
   int   FB32X32

   ldx   #$0f
   stx   FB32X32_WIDTH
   stx   FB32X32_HEIGHT
   stx   FB32X32_STEP
;   stz   FB32X32_SRC+0 ; not required due to fb setup

@newframe:
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
   tay                     ; sta   lefty
   eor   #$0f              ; mirror on X
   pha

   txa
   and   #$01
   beq   :+
   txa
   lsr
:
   sta   FRAMEBUFFER+$000,y
   ply
   sta   FRAMEBUFFER+$100,y

   inx
   cpx   #$10
   bcc   @loop

   inc   xp
   inc   yp
   inc   mp

   ldx   #$00
   ldy   #$ff
:
   lda   FRAMEBUFFER+$000,x
   sta   FRAMEBUFFER+$300,y
   lda   FRAMEBUFFER+$100,x
   sta   FRAMEBUFFER+$200,y
   dey
   inx
   bne   :-

:
   lda   fbhi,x
   sta   FB32X32_SRC+1
   lda   xpos,x
   sta   FB32X32_DEST_X
   lda   ypos,x
   sta   FB32X32_DEST_Y
   lda   copy,x
   tay
   sta   FB32X32_COPY,y
   inx
   cpx   #$04
   bne   :-

   jmp   @newframe


fbhi:
   .byte (>FRAMEBUFFER)+0,(>FRAMEBUFFER)+1,(>FRAMEBUFFER)+2,(>FRAMEBUFFER)+3
xpos:
   .byte              $00,             $10,             $00,             $10
ypos:
   .byte              $00,             $00,             $10,             $10
copy:
   .byte                1,               1,               1,               0
