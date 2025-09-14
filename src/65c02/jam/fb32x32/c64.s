
.include "fb32x32.inc"

vector := $10

FRAMEBUFFER := $cc00

.segment "CODE"

   ldx   #$00
:
   lda   gfx,x
   sta   gfx,x
   inx
   bne   :-

   ldy   #$1f
   
   lda   #>gfx
   sta   FB32X32_SRC+1
   stz   FB32X32_DEST_X
   stz   FB32X32_HEIGHT
   sty   FB32X32_WIDTH
   sty   FB32X32_STEP
:
   stx   FB32X32_DEST_Y
   lda   offsets,x
   sta   FB32X32_SRC+0
   stz   FB32X32_COPYN
   inx
   cpx   #$20
   bcc   :-

   jsr   PRINT
   .byte 10,"(Press Ctrl+C to quit) ",0

   lda   #FB32X32_CMAP_C64
   sta   FB32X32_COLMAP

   lda   #$02
   sta   FB32X32_WIDTH
   lda   #$0a
   sta   FB32X32_DEST_Y
   lda   #$80
:
   eor   #$a0
   sta   FB32X32_SRC+0
   stz   FB32X32_COPY
:
   inx
   bne   :-
   iny
   bne   :-
   
   pha
   jsr   CHRIN
   cmp   #$03
   beq   @end
   pla
   bra   :--

@end:
   jmp   ($fffc)

offsets:
   .byte $00,$00,$00,$00,$20,$40,$20,$60,$20,$80,$20,$20,$20,$20,$20,$20
   .byte $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$00,$00,$00,$00

.align $100

gfx:
   .byte $e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e
   .byte $e,$e,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$e,$e
   .byte $e,$e,$6,$6,$6,$e,$e,$e,$6,$e,$e,$e,$e,$e,$6,$e,$e,$e,$e,$6,$e,$e,$e,$6,$e,$e,$e,$6,$6,$6,$e,$e
   .byte $e,$e,$6,$e,$e,$6,$e,$e,$6,$e,$e,$e,$e,$6,$6,$e,$e,$e,$6,$e,$e,$e,$6,$e,$e,$e,$6,$e,$e,$6,$e,$e
   .byte $e,$e,$e,$e,$e,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$6,$e,$e
