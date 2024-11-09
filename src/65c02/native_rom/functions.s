
.include "../native.inc"
.include "../native_kernel.inc"
.include "../native_bios.inc"

.define INPUT_DEBUG 0

ESC      := $1b
VT_LEFT  := 'D'
VT_RIGHT := 'C'
VT_SAVE  := 's'
VT_UNSAVE:= 'u'

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

   jsr   printtmp16  ; returns with A=$00 and Y=index

   sty   PSAVE
:
   sta   (TMP16),y
   cpy   ASAVE
   iny
   bcc   :-
   bcs   @getloop

@bs:
   tya               ; cpx #$00
   beq   @getloop
   lda   #$08
   jsr   CHROUT

   lda   #VT_SAVE
   jsr   vtprint
   dey
:
   iny
   lda   (TMP16),y
   dey
   sta   (TMP16),y
   jsr   CHROUT
   iny
   cpy   ASAVE
   bcc   :-
   dec   PSAVE
   lda   #' '
   jsr   CHROUT
   lda   #VT_UNSAVE
   jsr   vtprint

@getloop:
.if INPUT_DEBUG
   jsr   debug
.endif

   jsr   getkey
   ldy   PSAVE

   cmp   #$c8
   beq   @chome
   cmp   #$c6
   beq   @cend
   cmp   #$c3
   beq   @cright
   cmp   #$c4
   beq   @cleft
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

   pha               ; save letter
   ; move buffer from INBUF+CPOS to input+CPOS+1
   ldy   ASAVE
   dey
   lda   (TMP16),y
   beq   :+
   pla               ; correct stack
   bne   @getloop    ; letter != $00
:
   iny
:
   dey
   lda   (TMP16),y
   iny
   sta   (TMP16),y
   dey
   cpy   PSAVE
   bne   :-
   pla               ; restore letter
   sta   (TMP16),y
   inc   PSAVE
   jsr   CHROUT      ; always returns n=0
   lda   #VT_SAVE
   jsr   vtprint
   iny
:
   lda   (TMP16),y
   beq   :+
   jsr   CHROUT
   iny
   bne   :-
:
   lda   #VT_UNSAVE
   jsr   vtprint
   bpl   @getloop    ; always true because of vtprint

@cleft:
   tya               ; cpy #$00
   beq   @getloop
   dec   PSAVE
   lda   #VT_LEFT
   jsr   vtprint
   bpl   @getloop    ; always true

@cright:
   lda   (TMP16),y
   beq   @getloop
   inc   PSAVE
   lda   #VT_RIGHT
   jsr   vtprint
   bpl   @getloop    ; always true

@chome:
   lda   #$08
   jsr   CHROUT
   dey
   bne   @chome
   sty   PSAVE
   jmp   @getloop

@cend:
   lda   (TMP16),y
   beq   :+
   jsr   CHROUT
   iny
   bne   @cend
:
   sty   PSAVE
   jmp   @getloop

@cancel:
   sec
   .byte $24
@okay:
   clc
   ldy   #$ff
:
   iny
   lda   (TMP16),y
   bne   :-
   sty   BRK_SY
rts0:
   rts

; this function returns:
; $00-$7f: ascii value of key
; $b2: insert
; $b3: delete
; $b5: page up
; $b6: page down
; $c1: up
; $c2: down
; $c3: right
; $c4: left
; $c6: end
; $c8: home
getkey:
   jsr   CHRIN
   bcs   getkey
   cmp   #ESC
   bne   @done

   ldy   #$00
:
   jsr   CHRIN
   cmp   #'['
   beq   :+
   iny
   bne   :-
   beq   @retesc     ; return the ESC
:
   jsr   CHRIN
   bcc   :+
   iny
   bne   :-
:
   cmp   #$40
   bcs   @conv
   pha
:
   jsr   CHRIN
   cmp   #'~'
   beq   :+
   iny
   bne   :-
   beq   @retesc1    ; return the ESC
:
   pla
@conv:
   ora   #$80
   bne   @done

@retesc1:
   pla
@retesc:
   lda   #ESC
@done:
   bit   BRK_SY
   bpl   rts0
   jmp   uppercase

vtprint:
   pha
   lda   #ESC
   jsr   CHROUT
   lda   #'['
   jsr   CHROUT
   pla
   jmp   CHROUT

.if INPUT_DEBUG
home:
   .byte ESC,"[s",ESC,"[1;1H",0

debug:
   ldy   #$00
:
   lda   home,y
   beq   :+
   jsr   CHROUT
   iny
   bne   :-
:
   lda   PSAVE
   jsr   prhex8
   lda   #' '
   jsr   CHROUT
   lda   ASAVE
   jsr   prhex8
   lda   #' '
   jsr   CHROUT

   ldy   #$00
:
   lda   (TMP16),y
   jsr   prhex8
   lda   #' '
   jsr   CHROUT
   iny
   cpy   ASAVE
   bne   :-

   lda   #VT_UNSAVE
   jmp   vtprint
.endif
