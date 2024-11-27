
; lineinput
; intended to run with any 6502 CPU, both NMOS and CMOS
;
; code needs to be valid using an NMOS 6502 as well, will also be used by
; NMOS 6502 toolkit
;
; input
; A/X: target lo/hi
; Y: bit7=0: size of buffer (+1 byte will be used for end-$00)
; Y: bit7=1: bit6-0 << 1 = zeropage address pointing to config block
;
; output:
; A: key used to end
; Y: size of input
; c=0: return, c=1: something else (Ctrl-C, CRSR-up/down, etc.)

.include "../native_bios.inc"
.include "../native.inc"
.include "../native_kernel.inc"

.export  inputline
.export  getkey

.define  INPUT_DEBUG 0

ESC      := $1b
VT_LEFT  := 'D'
VT_RIGHT := 'C'
VT_SAVE  := 's'
VT_UNSAVE:= 'u'

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
; Y: maxlength (up to 255 bytes)

   sta   TMP16+0
   stx   TMP16+1
   sty   ASAVE

   jsr   printtmp16  ; returns with A=$00 and Y=index

   sty   PSAVE
:
   sta   (TMP16),y
   cpy   ASAVE
   iny
   bcc   :-
   bcs   getloop

delete:
   ; implemented as "go right + backspace"
   cpy   ASAVE       ; is cursor at end?
   bcs   getloop     ; yes -> nothing to delete
   inc   PSAVE       ; internally stepping one to the right
   bcc   delcont     ; skip moving cursor

bs:
   tya               ; is cursor at start? (tya does cpy #$00)
   beq   getloop     ; yes -> nothing to backspace
   lda   #$08
   jsr   CHROUT      ; move cursor one to the left
   dey

delcont:
   lda   #VT_SAVE    ; save cursor position, so after printing
   jsr   vtprint     ; cursor is at same place
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
   lda   #' '        ; print space to delete last character
   jsr   CHROUT
   lda   #VT_UNSAVE
   jsr   vtprint

getloop:
.if INPUT_DEBUG
   jsr   debug
.endif
   clc
   jsr   getkey
   ldy   PSAVE

   ldx   #<(funcs-keys-1)
@loop:
   cmp   keys,x
   beq   @found
   dex
   bpl   @loop

   cmp   #' '
   bcc   getloop
   tax               ;check if A>$7F
   bmi   getloop

   pha               ; save letter
   ; move buffer from INBUF+CPOS to input+CPOS+1
   ldy   ASAVE
   dey
   lda   (TMP16),y
   beq   :+
   pla               ; correct stack
   bne   getloop    ; letter != $00
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
   bpl   getloop    ; always true because of vtprint

@found:
.ifp02
   txa
   asl
   tax
   lda   funcs+1,x
   pha
   lda   funcs+0,x
   pha
   txa
   lsr
   tax
   lda   keys,x
   rts
.else
   pha
   txa
   asl
   tax
   pla
   jmp   (funcs,x)
.endif
@next:


cleft:
   tya               ; cpy #$00
   beq   getloop
   dec   PSAVE
.if 1
   lda   #$08
   jsr   CHROUT
.else
   lda   #VT_LEFT
   jsr   vtprint
.endif
   bpl   getloop    ; always true

cright:
   lda   (TMP16),y
   beq   getloop
   inc   PSAVE
.if 1
   jsr   CHROUT
.else
   lda   #VT_RIGHT
   jsr   vtprint
.endif
   bpl   getloop    ; always true

chome:
   tya
   beq   @done
:
   lda   #$08
   jsr   CHROUT
   dey
   bne   :-
@done:
   sty   PSAVE
   jmp   getloop

cend:
   lda   (TMP16),y
   beq   :+
   jsr   CHROUT
   iny
   bne   cend
:
   sty   PSAVE
   jmp   getloop

deltoend:
   lda   #VT_SAVE
   jsr   vtprint

:
   lda   #' '
   jsr   CHROUT
   lda   #$00
   sta   (TMP16),y
   iny
   cpy   ASAVE
   bne   :-
   lda   #VT_UNSAVE
   jsr   vtprint

   jmp   getloop

leave:
   pha
   ldy   #$ff
:
   iny
   lda   (TMP16),y
   bne   :-

   pla
.ifp02
.else
   ; prepare interrupt return register
   sta   BRK_SA
   sty   BRK_SY
.endif
   cmp   #$0d        ; sets Z=1 if return was used to end
rts0:
   rts

keys:
   ;     RET  ^C ESC  BS DEL  ^A  ^E  ^K  ^D
   .byte $0d,$03,ESC,$08,$7f,$01,$05,$0b,$04

   ;     DELETE UP DOWN RIGHT LEFT END HOME
   .byte $b3,  $c1, $c2,  $c3, $c4,$c6, $c8

funcs:
.ifp02
   .word leave-1     ;RET
   .word leave-1     ;^C
   .word leave-1     ;ESC
   .word bs-1        ;BS
   .word bs-1        ;DEL
   .word chome-1     ;^A
   .word cend-1      ;^E
   .word deltoend-1  ;^K
   .word delete-1    ;^D
   .word delete-1    ;DELETE
   .word leave-1     ;UP
   .word leave-1     ;DOWN
   .word cright-1    ;RIGHT
   .word cleft-1     ;LEFT
   .word cend-1      ;END
   .word chome-1     ;HOME
.else
   .word leave       ;RET
   .word leave       ;^C
   .word leave       ;ESC
   .word bs          ;BS
   .word bs          ;DEL
   .word chome       ;^A
   .word cend        ;^E
   .word deltoend    ;^K
   .word delete      ;^D
   .word delete      ;DELETE
   .word leave       ;UP
   .word leave       ;DOWN
   .word cright      ;RIGHT
   .word cleft       ;LEFT
   .word cend        ;END
   .word chome       ;HOME
.endif

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
   php               ; save carry
:
   jsr   CHRIN
   bcs   :-
   cmp   #ESC
   bne   @done

   ldy   #$00
:
   jsr   CHRIN
   cmp   #'['
   beq   :+
   cmp   #'O'
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
   plp               ; restore carry flag
   bcc   rts0        ; clear -> no uppercase

uppercase:
   cmp   #'a'
   bcc   :+
   cmp   #'z'+1
   bcs   :+
   and   #$df
:
   rts

vtprint:
   pha
   lda   #ESC
   jsr   CHROUT
   lda   #'['
   jsr   CHROUT
   pla
   jmp   CHROUT

.if INPUT_DEBUG
debug:
   ldy   #$00
:
   lda   @home,y
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

@home:
   .byte ESC,"[s",ESC,"[1;1H",0
.endif
