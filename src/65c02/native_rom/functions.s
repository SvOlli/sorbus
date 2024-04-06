
.include "../native.inc"
.include "../native_kernel.inc"
.include "../native_bios.inc"

valuei := PSAVE ; value integer part
valued := ASAVE ; value decimal places
deltai := $08   ; delta integer part
deltad := $09   ; delta decimal places

index0 := $0a
index1 := $0b
index2 := $0c
index3 := $0d

ovec   := TMP16
max    := PSAVE

gensine:
; A: page for table
; X: size ($01-$10)
; Y: variant
   sta   ovec+1
   tya
   ror
   ror
   ror
   and   #$c0    ; move start
   sta   index0
   clc
   adc   #$7f
   sta   index1
   inc
   sta   index2
   clc
   adc   #$7f
   sta   index3

   stz   valuei  ; value integer part
   stz   valued  ; value decimal places
   stz   deltai  ; delta integer part
   stz   deltad  ; delta decimal places

   ldx   #$3f
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

; could use bbs2 here, but that would break 65816 compatibility
.if 0
   bbs2  BRK_SY(needs to be copied to zeropage),:+
.else
   lda   BRK_SY
   and   #$04
   beq   :+

   lda   valued
.endif
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
   bpl   @parloop

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

printtmp16:
   ldy   #$00
:
   lda   (TMP16),y
   beq   :+
   jsr   CHROUT
   iny
   bra   :-
:
   rts

inputline:
; A/X: buffer
; Y: maxlength
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

   cmp   #' '
   bcc   @getloop

   cpy   ASAVE
   bcs   @getloop

   sta   (TMP16),y
   jsr   CHROUT
   iny
   bra   @getloop

@bs:
   tya                ;cmp #$00
   beq   @getloop
   lda   #$7f
   jsr   CHROUT
   dey
   bra   @getloop

@cancel:
   sec
   rts
@okay:
   clc
   rts

