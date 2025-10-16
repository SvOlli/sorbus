; super mega amiga emulator :) :) :)
; (c)by Thorex

.include "fb32x32.inc"

.define FRAMEBUFFER $cc00

.define DELAY 10000

  sei
  lda #<FRAMEBUFFER
  ldx #>FRAMEBUFFER
  ldy #$00
  int FB32X32
  lda #FB32X32_CMAP_C64
  sta FB32X32_COLMAP
  ldx #$00
:
  stz FRAMEBUFFER+$000,x
  stz FRAMEBUFFER+$100,x
  stz FRAMEBUFFER+$200,x
  stz FRAMEBUFFER+$300,x
  inx
  bne :-
  stz FB32X32_COPY

  lda #<irqhandler
  sta UVNBI+0
  lda #>irqhandler
  sta UVNBI+1

  lda #<DELAY
  sta TMIMRL
  lda #>DELAY
  sta TMIMRH
  cli

  ldy #$04
:
  cpy #$00
  bne :-
  sei
  stz TMIMRL
  stz TMIMRH
  jmp ($fffc)

irqhandler:
  dey
  lda coltab,Y
  beq floppy

  ldx #0
:
  sta FRAMEBUFFER+$000,x
  sta FRAMEBUFFER+$100,x
  sta FRAMEBUFFER+$200,x
  sta FRAMEBUFFER+$300,x
  dex
  bne :-
  beq irqdone

floppy:
  ldx #0
:
  lda pic1,x
  sta FRAMEBUFFER+$000,x
  lda pic2,x
  sta FRAMEBUFFER+$100,x
  lda pic3,x
  sta FRAMEBUFFER+$200,x
  lda pic4,x
  sta FRAMEBUFFER+$300,x
  dex
  bne :-

irqdone:
  stz FB32X32_COPY
  lda TMIMRL
  rti

coltab:
  .byte $0,$1,$f,$b


pic1:
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1
        .byte 1,1,1,1,1,1,1,0,$e,$e,$e,$e,0,$f,$f,$f
        .byte $f,$f,$f,$f,0,0,0,$f,0,$e,$e,0,1,1,1,1
        .byte 1,1,1,1,1,1,1,0,$e,$e,$e,$e,0,$f,$f,$f
        .byte $f,$f,$f,$f,0,$e,0,$f,0,$e,$e,$e,0,1,1,1
        .byte 1,1,1,1,1,1,1,0,$e,$e,$e,$e,0,$f,$f,$f
        .byte $f,$f,$f,$f,0,$e,0,$f,0,$e,$e,$e,$e,0,1,1
        .byte 1,1,1,1,1,1,1,0,$e,$e,$e,$e,0,$f,$f,$f
        .byte $f,$f,$f,$f,0,0,0,$f,0,$e,$e,$e,$e,0,1,1
        .byte 1,1,1,1,1,1,1,0,$e,$e,$e,$e,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,$e,$e,$e,$e,0,1,1

pic2:
        .byte 1,1,1,1,1,1,1,0,$e,$e,$e,$e,$e,$e,$e,$e
        .byte $e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,0,1,1
        .byte 1,1,1,1,1,1,1,0,$e,$e,$e,$e,$e,$e,$e,$e
        .byte $e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,0,1,1
        .byte 1,1,1,1,1,1,1,0,$e,$e,$e,$e,$e,$e,$e,$e
        .byte $e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,0,1,1
        .byte 1,1,1,1,1,1,1,0,$e,$e,$e,$e,$e,$e,$e,$e
        .byte $e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,$e,0,1,1
        .byte 1,1,1,1,1,1,1,0,$e,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,$e,$e,0,1,1
        .byte 1,1,1,1,1,1,0,0,$e,0,0,1,1,1,1,1
        .byte 1,1,1,1,1,1,1,1,1,1,0,$e,$e,0,1,1
        .byte 1,1,1,1,1,0,1,0,$e,0,0,0,0,1,1,$e
        .byte $e,$e,1,$e,1,$e,1,$e,1,1,0,$e,$e,0,1,1
        .byte 1,1,1,0,0,1,1,0,0,1,0,1,1,0,1,1
        .byte 1,1,1,1,1,1,1,1,1,1,0,$e,$e,0,1,1

pic3:
        .byte 1,1,0,1,1,1,0,1,1,1,1,0,0,1,1,1
        .byte $e,1,$e,1,$e,1,$e,1,1,1,0,$e,$e,0,1,1
        .byte 1,0,1,1,0,1,1,1,1,1,1,1,0,1,1,0
        .byte 1,1,1,1,1,1,1,1,1,1,0,$e,$e,0,1,1
        .byte 1,0,1,0,1,1,1,1,1,1,0,0,1,1,0,$f
        .byte 0,1,0,1,1,1,0,0,0,1,0,$e,$e,0,1,1
        .byte 1,0,1,1,1,1,1,1,1,1,0,1,1,1,1,0
        .byte 0,$f,1,0,1,$f,1,0,1,1,0,$e,$e,0,1,1
        .byte 1,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1
        .byte 1,0,1,1,1,1,1,1,1,1,1,0,1,1,0,0
        .byte 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,0,1,1,1,1,1,1,1,1,1,0,1,0,0,1
        .byte 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,0,1,1,1,1,1,1,1,1,1,0,1,0,1,1
        .byte 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1

pic4:
        .byte 1,0,1,1,1,1,1,1,1,1,1,0,1,0,0,0
        .byte 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,0,1,1,1,1,1,1,0,0,0,0,0,1,1,0
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,0,1,1,1,1,1,1,0,1,1,1,0,0,0,1
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,1
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,1
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,1,0,1,1,1,1,1,0,1,1,1,0,1,1,1
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
        .byte 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
