
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
BARS        := $c000       ; must be page aligned

start:
   jmp   init

screenupdate:

irqhandler:
   pha
   phx
   phy

   jsr   screenupdate
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
   stz   FB32X32_COLMAP ; default rgbi2222

   lda   #$07
   sta   chroma
   stz   luma

   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$01
   int   FB32X32

   sei
   lda   #<irqhandler
   sta   UVNBI+0
   lda   #>irqhandler
   sta   UVNBI+1
   lda   #<DELAY
   sta   TMIMRL
   lda   #>DELAY
   sta   TMIMRH

   stz   vector+0
   lda   #>BARS
   sta   vector+1


   lda   #<(colordataend-colordata)
   sta   tmp8

@preploop:

   tya
   and   #$03
   ora   tmp8
   tax

   lda   tmp8
   sec
   sbc   #$04
   sta   tmp8
   bne   @preploop

   cli

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

colordata:
   ; red
   .byte $40,$80,$c0,$c3
   ; green
   .byte $10,$20,$30,$33
   ; blue
   .byte $04,$08,$0c,$0f
   ; yellow (red+green)
   .byte $50,$a0,$f0,$f3
   ; purple (red+blue)
   .byte $44,$88,$cc,$c3
   ; cyan (green+blue)
   .byte $14,$28,$3c,$3f
   ; white
   .byte $54,$a8,$fc,$ff
colordataend:

