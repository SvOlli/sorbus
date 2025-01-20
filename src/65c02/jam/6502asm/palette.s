
.include "fb32x32.inc"

vector := $10

FRAMEBUFFER := $cc00

main:
   jsr   PRINT
   .byte 10,"Known colormaps:"
   .byte 10,"0: Insane (RGBI2222)"
   .byte 10,"4: C64 (16 colors)"
   .byte 10,"5: C16 (128 colors)"
   .byte 10,"6: Atari 8-bit (256 colors)"
   .byte 10,"Press 0-9 to display colormap (CTRL-C to quit) ",0

   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$01
   int   FB32X32

:
   tya
   sta   FRAMEBUFFER,y
   iny
   bne   :-

   lda   #$08           ; startpos x, y
   sta   FB32X32_DEST_X
   sta   FB32X32_DEST_Y
   lda   #$0f           ; witdh, height, step: 16 pixel
   sta   FB32X32_WIDTH
   sta   FB32X32_HEIGHT
   sta   FB32X32_STEP

@update:
   stz   FB32X32_COPY
:
   jsr   CHRIN
   bcs   :-
   cmp   #$03
   beq   @done
   cmp   #'0'
   bcc   :-
   cmp   #'9'+1
   bcs   :-
   and   #$0f
   sta   FB32X32_COLMAP
   bra   @update

@done:
   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$01
   int   FB32X32
   jmp   ($fffc)
