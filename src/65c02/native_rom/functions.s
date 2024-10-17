
.include "../native.inc"
.include "../native_kernel.inc"
.include "../native_bios.inc"

valuei := PSAVE      ; value integer part
valued := ASAVE      ; value decimal places

; need to be saved
savest := $08

deltai := $08        ; delta integer part
deltad := $09        ; delta decimal places

index0 := $0a
index1 := $0b
index2 := $0c
index3 := $0d

savend := $0e

ovec   := TMP16

; ===========================================================================

gensine:
; A: page for table
; X: size ($01-$10), taken from BRK_SX
; Y: variant ($00-$03), add $04 to write decimal parts to following page

   ; we need quite some zeropage variables
   ; save previous values on the stack

   stz   ovec+0
   sta   ovec+1
   ldx   #(savend-savest-1)
:
   lda   savest,x
   pha
   dex
   bpl   :-

   tya
   ror
   ror
   ror
   and   #$c0        ; move start
   sta   index0
   clc
   adc   #$7f
   sta   index1
   inc
   sta   index2
   clc
   adc   #$7f
   sta   index3

   stz   valuei      ; value integer part
   stz   valued      ; value decimal places
   stz   deltai      ; delta integer part
   stz   deltad      ; delta decimal places

   ldx   #$40        ; just a counter
@parloop:
   phx
   clc
   lda   valued
   adc   deltad
   sta   valued
   lda   valuei
   adc   deltai
   sta   valuei

   lda   BRK_SX
   asl
   asl
   asl
   asl
   dec

   sec
   sbc   valuei
   ldx   valuei
   jsr   @store4q

   lda   BRK_SY
   and   #$04
   beq   :+

   ; bit 2 is set -> store decimal part in following page
   lda   valued
   inc   ovec+1
   lda   #$ff
   sec
   sbc   valued
   ldx   valued
   jsr   @store4q
   dec   ovec+1
:

   clc
   lda   deltad
   adc   BRK_SX
   sta   deltad
   bcc   :+
   inc   deltai
:

   inc   index0
   dec   index1
   inc   index2
   dec   index3
   plx
   dex
   bne   @parloop

   ; restore zeropage variables used
   ;ldx   #$00        ; X=0 already
:
   pla
   sta   savest,x
   inx
   cpx   #(savend-savest)
   bcc   :-

   rts

@store4q:
   ldy   index0
   sta   (ovec),y
   ldy   index3
   sta   (ovec),y
   txa
   ldy   index1
   sta   (ovec),y
   ldy   index2
   sta   (ovec),y
   rts

; ===========================================================================

printtmp16:
   ; TODO: decide if this is reusable

   ldy   #$00
:
   lda   (TMP16),y
   beq   :+
   jsr   CHROUT
   iny
   bne   :-
:
   rts

; ===========================================================================

inputline:
; A/X: buffer
; Y: maxlength (up to 127 bytes) (bit 7: convert to uppercase)

   sta   TMP16+0
   stx   TMP16+1
   tya
   and   #$7f
   sta   ASAVE

   jsr   printtmp16

@getloop:
   jsr   CHRIN
   beq   @getloop

   bit   BRK_SY
   bpl   :+
   jsr   uppercase
:
   cmp   #$08
   beq   @bs
   cmp   #$7f
   beq   @bs
   bcs   @getloop
   cmp   #$03
   beq   @cancel
   cmp   #$0d
   beq   @okay

   cpy   ASAVE
   bcs   @getloop

   cmp   #' '
   bcc   @getloop

   sta   (TMP16),y
   jsr   CHROUT
   iny
   bra   @getloop

@bs:
   tya               ; cmp #$00
   beq   @getloop
   lda   #$7f
   jsr   CHROUT
   dey
   bra   @getloop

@cancel:
   sec
   .byte $24
@okay:
   clc
   php
   sty   BRK_SY
   lda   #$00
@clrloop:
   cpy   ASAVE
   bcs   @clrdone
   sta   (TMP16),y
   iny
   bne   @clrloop
@clrdone:
   plp
   rts
