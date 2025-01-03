
;-------------------------------------------------------------------------
;  WozMon for JAM core
;-------------------------------------------------------------------------

;-------------------------------------------------------------------------
; This code is mostly written and copyright by
; Apple Computer Company in 1976
;-------------------------------------------------------------------------
; WozMon
; Code taken from Apple 1 ROMs and heavily modified:
; - replaced code for I/O with Sorbus kernel calls
; - adapted some of the code from 6502 to 65C02 assembler
;-------------------------------------------------------------------------
; Code needs to reside in bank 1, since it's run from NMOS 6502 as well
;-------------------------------------------------------------------------

.include "jam_bios.inc"
.include "jam_kernel.inc"
.segment "CODE"

;-------------------------------------------------------------------------
;  Memory declaration
;-------------------------------------------------------------------------

.if 1
.define  USE16COLS   1
.define  USE65C02    0
XAML           :=     $08            ; Last "opened" location Low
XAMH           :=     $09            ; Last "opened" location High
STL            :=     $0A            ; Store address Low
STH            :=     $0B            ; Store address High
L              :=     $0C            ; Hex value parsing Low
H              :=     $0D            ; Hex value parsing High
YSAV           :=     $0E            ; Used to see if hex value is given
MODE           :=     $0F            ; $00=XAM, $7F=STOR, $AE=BLOCK XAM
.else
; original WozMon addresses for reference
.define  USE16COLS   0
.define  USE65C02    0
XAML           :=     $24            ; Last "opened" location Low
XAMH           :=     $25            ; Last "opened" location High
STL            :=     $26            ; Store address Low
STH            :=     $27            ; Store address High
L              :=     $28            ; Hex value parsing Low
H              :=     $29            ; Hex value parsing High
YSAV           :=     $2A            ; Used to see if hex value is given
MODE           :=     $2B            ; $00=XAM, $7F=STOR, $AE=BLOCK XAM
.endif

IN              :=     $0200 ;,$027F     Input buffer

; KBD b7..b0 are inputs, b6..b0 is ASCII input, b7 is constant high
;     Programmed to respond to low to high KBD strobe
; DSP b6..b0 are outputs, b7 is input
;     CB2 goes low when data is written, returns high when CB1 goes high
; Interrupts are enabled, though not used. KBD can be jumpered to IRQ,
; whereas DSP can be jumpered to NMI.

;-------------------------------------------------------------------------
;  Constants
;-------------------------------------------------------------------------

DEL             :=     $7f            ; DEL: interpreted as backspace
BS              :=     $08            ; "^H": interpreted as backspace
CR              :=     $0d            ; carriage return
LF              :=     $0a            ; linefeed
ESC             :=     $1b            ; ESC key
PROMPT          :=     $5c            ; "\": prompt character

;-------------------------------------------------------------------------
;  Let's get started
;
;  Remark the RESET routine is only to be entered by asserting the RESET
;  line of the system. This ensures that the data direction registers
;  are selected.
;-------------------------------------------------------------------------

wozstart:
   cli                  ; required for testing IRQs
   lda   #ESC           ; KBD and DSP control register mask

; Program falls through to the GETLINE routine to save some program bytes
; Please note that Y still holds $7F, which will cause an automatic Escape

;-------------------------------------------------------------------------
; The GETLINE process
;-------------------------------------------------------------------------

notcr:
   cmp   #DEL           ; check for alternative backspace key
   beq   backspace      ;   and jump to handling
   cmp   #BS            ; check for backspace key
   beq   backspace      ;   and jump to handling
   cmp   #ESC           ; check for escape key
   beq   escape         ;   and jump to handling
   iny                  ; advance to next char in buffer
   bpl   nextchar       ; treat input buffer overflow like escape key

escape:
   lda   #PROMPT
   jsr   CHROUT         ; print prompt character

getline:
   lda   #LF
   jsr   CHROUT         ; print newline

   ldy   #$01           ; start a new input, 1 compensates next line

backspace:
   dey
   bmi   getline        ; buffer underflow -> restart

nextchar:
   jsr   chrinuc        ; was: int   CHRINUC
   sta   IN,y           ; add to buffer
   jsr   CHROUT         ; print character

   cmp   #CR
   bne   notcr          ; if it's not return loop

; Line received, now let's parse it

   ldy   #$ff           ; reset input index
   lda   #$00           ; default mode is XAM
   tax

setstore:
   asl

setmode:
   sta   MODE           ; set mode flags

blskip:
   iny

nextitem:
   lda   IN,y           ; get character from buffer
   cmp   #CR            ; CR -> end of line
   beq   getline
   ora   #$80           ; simulate Apple 1 behaviour on Sorbus
   cmp   #$ae           ; check for '.'
   bcc   blskip         ; skip everything below '.', e.g. space
   beq   setmode        ; '.' sets block XAM mode
   cmp   #$ba           ; check for ':'
   beq   setstore       ; set STOR mode $
   and   #$5f
   cmp   #'R'           ; check for 'R'
   beq   run
   cmp   #'G'           ; check for 'G' (same as 'R', as in Apple ][)
   beq   run
   stx   L
   stx   H
   sty   YSAV

nexthex:
   lda   IN,y           ; get character for hex test
   cmp   #'0'
   bcc   nohex
   cmp   #'G'           ; $41-$46
   bcs   nohex
   cmp   #':'           ; test for digit
   bcc   dig            ; it is a digit
   adc   #$b8           ; adjust for A-F
   cmp   #$fa           ; test for hex letter
   bcc   nohex          ; it isn't a hex letter

dig:
   asl
   asl
   asl
   asl

   ldx   #$04           ; process 4 bits
hexshift:
   asl                  ; move most significant bit into carry
   rol   L              ; rotate carry through 16 bit address lobyte
   rol   H              ; ...hibyte
   dex
   bne   hexshift
   iny
   bne   nexthex

nohex:
   cpy   YSAV           ; at least one digit in buffer
   beq   escape         ; no -> restart

   bit   MODE           ; check top 2 bits of mode
   bvc   notstore       ; bit 6: 0 is STOR, 1 is XAM or block XAM

; STOR mode, save LSD of new hex byte

   lda   L              ; LSD of hex data
.if USE65C02
   sta   (STL)
.else
   sta   (STL,x)        ; X=0 -> store at address in STL
.endif
   inc   STL
   bne   nextitem
   inc   STH
tonextitem:
   jmp   nextitem

;-------------------------------------------------------------------------
;  RUN user's program from last opened location
;-------------------------------------------------------------------------

run:
   jmp   (XAML)         ; execute supplied address

;-------------------------------------------------------------------------
;  We're not in Store mode
;-------------------------------------------------------------------------

notstore:
   bmi   xamnext        ; bit 7: 0=XAM, 1=block XAM

; We're in XAM mode now
   ldx   #$02           ; copy 2 bytes
setaddr:
   lda   L-1,x          ; copy hex data to
   sta   STL-1,x        ; ..."store index"
   sta   XAML-1,x       ; ...and "XAM index"
   dex
   bne   setaddr

; Print address and data from this address, fall through next BNE.

nextprint:
   bne   prdata         ; no address to print
   lda   #LF
   jsr   CHROUT         ; start new line
   lda   XAMH
   jsr   prhex8         ; print hibyte of address
   lda   XAML
   jsr   prhex8         ; print lobyte of address
   lda   #':'           ; ":"
   jsr   CHROUT         ; print colon

prdata:
.if USE16COLS
   cmp   #$08           ; print extra space on 8th column
   bne   :+
   lda   #' '           ; " "
   jsr   CHROUT         ; print space
:
.endif
   lda   #' '           ; " "
   jsr   CHROUT         ; print space
.if USE65C02
   lda   (XAML)         ; get data from address
.else
   lda   (XAML,x)       ; get data from address
.endif
   jsr   prhex8         ; print byte

xamnext:
   stx   MODE           ; set mode to XAM
   lda   XAML           ; check if
   cmp   L              ; ...there's
   lda   XAMH           ; ...more to
   sbc   H              ; ...print
   bcs   tonextitem

   inc   XAML
   bne   :+
   inc   XAMH
:
   lda   XAML
.if USE16COLS
   and   #$0f           ; start new line every 16 addresses
.else
   and   #$07           ; start new line every 8 addresses
.endif
   bpl   nextprint      ; jmp

;-------------------------------------------------------------------------
;  WozMon end
;-------------------------------------------------------------------------

.out "   =================="
.out .sprintf( "   WozMon size: $%04x", * - wozstart )
.out "   =================="
