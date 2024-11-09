
.P02
.include "../native.inc"
.include "../native_bios.inc"
.include "../native_kernel.inc"

.define INPUT_DEBUG 0

.import wozstart
.import Monitor

.export inputline
.export prhex16
.export prhex4
.export prhex8
.export prhex8s

ESC      := $1b
VT_LEFT  := 'D'
VT_RIGHT := 'C'
VT_SAVE  := 's'
VT_UNSAVE:= 'u'

.segment "CODE"
reset:
   jmp   start
   .byte "SBC23"

   jmp   inputtest
   jmp   getkeytest
;   jmp   Monitor

knownids:
   .byte $00,$01,$02,$06,$0E,$12,$21
cpunames:
   .byte "??",0,0
   .byte "02",0,0
   .byte "SC02"
   .byte "C02",0
   .byte "CE02"
   .byte "816",0
   .byte "65RA"

vectab:
   .word $0000          ; UVBRK: (unused) IRQ handler dispatches BRK
   .word uvnmi          ; UVNMI: hardware NMI handler
   .word $0000          ; UVNBI: (unused) IRQ handler dispatches non-BRK
   .word uvirq          ; UVIRQ: hardware IRQ handler

uvnmi:
uvirq:
   cld
   sta   TRAP
   rti

start:
   cld
   sei
   ldx   #$07           ; move to BIOS code?
:
   lda   vectab,x
   sta   UVBRK,x        ; setup user vectors
   dex
   bpl   :-

   txs                  ; initialize stack to $FF

   jsr   PRINT
   .byte 10,"CPU features: ",0
                   ; $01 NMOS, $02 CMOS, $04 BIT (RE)SET, $08 Z reg, $10 16 bit
   lda   CPUID
   lsr
   bcc   :+
   jsr   PRINT
   .byte "NMOS ",0
:
   lsr
   bcc   :+
   jsr   PRINT
   .byte "CMOS ",0
:
   lsr
   bcc   :+
   jsr   PRINT
   .byte "BBBSR ",0
:
   lsr
   bcc   :+
   jsr   PRINT
   .byte "Z-Reg ",0
:
   lsr
   bcc   :+
   jsr   PRINT
   .byte "16bit ",0
:
   jsr   PRINT
   .byte "=> 65",0

   lda   CPUID
   ldx   #<(cpunames-knownids)
:
   dex
   beq   :+
   cmp   knownids,x
   bne   :-
:
   txa
   asl
   asl
   tax
   ldy   #$04
:
   lda   cpunames,x
   beq   :+
   jsr   CHROUT
   inx
   dey
   bne   :-
:

woz:
   lda   #$0a           ; start WozMon port
   jsr   CHROUT
:  ; workaround for WozMon not handling RTS when executing external code
   jsr   wozstart
   ; will be reached if own code run within WozMon exits using rts
   jmp   :-             ; no bra here: NMOS 6502 fallback mode

chrinuc:
   ; wait for character from UART and make it uppercase
   jsr   CHRIN
   bcs   chrinuc
uppercase:
   cmp   #'a'
   bcc   :+
   cmp   #'z'+1
   bcs   :+
   and   #$df
:
   rts

prhex8s:
   ; print hex byte in A without modifying A (required by WozMon2)
   pha
   jsr   prhex8
   pla
   rts
prhex16:
   ; output 16 bit value in X,A
   pha
   txa
   jsr   prhex8
   pla
   ; fall through
prhex8:
   ; output 8 bit value in A
   pha                  ; save A for LSD
   lsr                  ; move MSD down to LSD
   lsr
   lsr
   lsr
   jsr   prhex4         ; print MSD
   pla                  ; restore A for LSD
prhex4:
   and   #$0f           ; mask LSD for hex PRINT
   ora   #'0'           ; add ascii "0"
   cmp   #':'           ; is still decimal
   bcc   :+             ; yes -> output
   adc   #$06           ; adjust offset for letters A-F
:
   jmp   CHROUT


; ===========================================================================

printtmp16:
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
   sty   BRK_SY
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

inputtest:
   jsr   PRINT
   .byte 10,"prompt> ",0
   lda   #$00
   sta   $0200
   ldx   #$02
   ldy   #$0f
   jsr   inputline
   jmp   $e000

getkeytest:
   jsr   getkey
   jsr   prhex8
   lda   #' '
   jsr   CHROUT
   jmp   getkeytest

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
