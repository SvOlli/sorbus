
.include "fb32x32.inc"

.define DELAY 500
.define DEBUGOUTPUT 0

ROTATE16TO32MATRIX := $cb00
FRAMEBUFFER        := $cc00

.segment "DATA"

colortab:
   .byte $0e,$1e,$2e,$3e,$4e,$5e,$6e,$7e,$8e,$9e,$ae,$be,$ce,$de,$ee,$fe
   .byte $0e,$1e,$2e,$3e,$4e,$5e,$6e,$7e,$8e,$9e,$ae,$be,$ce,$de,$ee,$fe

localram    = $10
pos         = localram+$00
idx         = localram+$01
colidx      = localram+$02
tmp8        = localram+$03

bl_x0       = localram+$0a
bl_y0       = localram+$0b
bl_x1       = localram+$0c
bl_y1       = localram+$0d
dx          = localram+$0e
dy          = localram+$0f

bresensteps = localram+$10 ; size=$10

.segment "CODE"

start:
   jsr   rotor_init

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

   jsr   rotor_run
   lda   TMIMRL         ; acknoledge timer

   ply
   plx
   pla
   rti


bresenhelp:
   lda   bl_x1
   sec
   sbc   bl_x0
   bcs   :+
   eor   #$ff
   adc   #$01              ; clc due to branch
:
   sta   dx

   lda   bl_y1
   sec
   sbc   bl_y0
   bcs   :+
   eor   #$ff
   adc   #$01              ; clc due to branch
:
   sta   dy

   lda   dx
   cmp   dy
   bcs   :+
   ; dx > dy -> swap
   lda   bl_x0
   ldx   bl_x1
   stx   bl_x0
   sta   bl_x1
   lda   bl_y0
   ldx   bl_y1
   stx   bl_y0
   sta   bl_y1
   lda   dx
   ldx   dy
   stx   dx
   sta   dy
:
   ldx   dx

   asl   dx
   asl   dy
@loop:
   tay                     ; dummy to set flags according to A
   stz   bresensteps,x     ; initial: clear out to $00
   bmi   @nostep
   dec   bresensteps,x     ; set to $FF
   sec
   sbc   dx
@nostep:
   clc
   adc   dy

   dex
   bpl   @loop
   rts


rotor_init:
   ldx   #$00
   lda   #$80
:
   sta   FRAMEBUFFER,x
   inx
   bne   :-

   lda   #FB32X32_CMAP_A800
   sta   FB32X32_COLMAP

   lda   #$0f
   ldy   #$00
@loop2:
   clc
@loop1:
   sta   ROTATE16TO32MATRIX,y
   iny
   adc   #$10
   bcc   @loop1
   dec
   cpy   #$00
   bne   @loop2
   rts


rotor_run:
   jsr   rotor_darken
   jsr   rotor_turn
   jsr   rotor_turn

   ldx   #$00
:
   lda   ROTATE16TO32MATRIX,x
   tay
   lda   FRAMEBUFFER+$000,x
   sta   FRAMEBUFFER+$100,y
   inx
   bne   :-

   ldy   #$ff
:
   lda   FRAMEBUFFER+$000,x
   sta   FRAMEBUFFER+$300,y
   lda   FRAMEBUFFER+$100,x
   sta   FRAMEBUFFER+$200,y
   dey
   inx
   bne   :-

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


rotor_darken:
   ldx   #$00
@darkenloop:
   lda   FRAMEBUFFER,x
   and   #$0f
   beq   @next
   dec   FRAMEBUFFER,x
@next:
   inx
   bne   @darkenloop
   rts


rotor_turn:
   lda   colidx
   dec
   sta   colidx
   lsr
   ;lsr
   and   #$0f
   clc
   adc   #<colortab
   sta   @lineloop+1
   lda   #>colortab
   adc   #$00
   sta   @lineloop+2

   lda   idx
   and   #$10
   bne   @part2

   lda   idx
   and   #$0f
   eor   #$0f              ; frame 0 starts at pos y0=15
   stz   bl_x0             ; x0=0
   sta   bl_y0
   asl
   asl
   asl
   asl
   sta   pos
   lda   #$0f
   sta   bl_x1
   sta   bl_y1

   jsr   bresenhelp

   ldx   #$0f
:
   lda   bresensteps,x
   and   #$10
   ora   #$01
   sta   bresensteps,x
   dex
   bpl   :-
   bra   @draw

@part2:
   lda   idx
   and   #$0f
   sta   bl_x0
   sta   pos
   stz   bl_y0
   lda   #$0f
   sta   bl_x1
   sta   bl_y1

   jsr   bresenhelp

   ldx   #$0f
:
   lda   bresensteps,x
   and   #$01
   ora   #$10
   sta   bresensteps,x
   dex
   bpl   :-

@draw:
   lda   idx
   lsr
   and   #$70
   ora   #$8f
   sta   tmp8

   ldx   #$0e
   ldy   pos
@lineloop:
   lda   colortab,x
   sta   FRAMEBUFFER,y

   tya
   clc
   adc   bresensteps,x
   tay

   dex
   bpl   @lineloop

   inx
   lda   colortab,x
   sta   FRAMEBUFFER,y

   dec   idx
   lda   pos
   bne   :+
   dec   idx
:
   rts
