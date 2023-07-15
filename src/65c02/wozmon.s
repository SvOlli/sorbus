;-------------------------------------------------------------------------
;
;  The WOZ Monitor for the Apple 1
;  Written by Steve Wozniak 1976
;
;-------------------------------------------------------------------------

.segment "CODE"
;-------------------------------------------------------------------------
;  Memory declaration
;-------------------------------------------------------------------------

XAML            :=     $24            ; Last "opened" location Low
XAMH            :=     $25            ; Last "opened" location High
STL             :=     $26            ; Store address Low
STH             :=     $27            ; Store address High
L               :=     $28            ; Hex value parsing Low
H               :=     $29            ; Hex value parsing High
YSAV            :=     $2A            ; Used to see if hex value is given
MODE            :=     $2B            ; $00=XAM, $7F=STOR, $AE=BLOCK XAM

IN              :=     $0200 ;,$027F     Input buffer

KBD             :=     $D010          ; PIA.A keyboard input
KBDCR           :=     $D011          ; PIA.A keyboard control register
DSP             :=     $D012          ; PIA.B display output register
DSPCR           :=     $D013          ; PIA.B display control register

; KBD b7..b0 are inputs, b6..b0 is ASCII input, b7 is constant high
;     Programmed to respond to low to high KBD strobe
; DSP b6..b0 are outputs, b7 is input
;     CB2 goes low when data is written, returns high when CB1 goes high
; Interrupts are enabled, though not used. KBD can be jumpered to IRQ,
; whereas DSP can be jumpered to NMI.

;-------------------------------------------------------------------------
;  Constants
;-------------------------------------------------------------------------

BS              :=     $DF            ; "_": interpreted as backspace
CR              :=     $8D            ; carriage return
ESC             :=     $9B            ; ESC key
PROMPT          :=     $DC            ; "\": prompt character

;-------------------------------------------------------------------------
;  Let's get started
;
;  Remark the RESET routine is only to be entered by asserting the RESET
;  line of the system. This ensures that the data direction registers
;  are selected.
;-------------------------------------------------------------------------

start:
   cld            ; just initializing bare minimum
   cli            ; note: not even stack pointer
   ldy   #$7f     ; mask DSP data direction register, also overflows buffer
   sty   DSP
   lda   #$a7     ; KBD and DSP control register mask
   sta   KBDCR    ; enable interrupts, set CA1, CB1 for positive sense
   sta   DSPCR    ;   output mode

; Program falls through to the GETLINE routine to save some program bytes
; Please note that Y still holds $7F, which will cause an automatic Escape

;-------------------------------------------------------------------------
; The GETLINE process
;-------------------------------------------------------------------------

notcr:
   cmp   #BS         ; check for backspace key
   beq   backspace   ;   and jump to handling
   cmp   #ESC        ; check for escape key
   beq   escape      ;   and jump to handling
   iny               ; advance to next char in buffer
   bpl   nextchar    ; treat input buffer overflow like escape key

escape:
   lda   #PROMPT
   jsr   echo     ; print prompt character

getline:
   lda   #CR
   jsr   echo     ; print newline

   ldy   #$01     ; start a new input, 1 compensates next line

backspace:
   dey
   bmi   getline  ; buffer underflow -> restart

nextchar:
   lda   KBDCR    ; check input
   bpl   nextchar ; no input -> loop

   lda   KBD      ; get key
   sta   IN,y     ; add to buffer
   jsr   echo     ; print character

   cmp   #CR
   bne   notcr    ; if it's not return loop

; Line received, now let's parse it

   ldy   #$ff     ; reset input index
   lda   #$00     ; default mode is XAM
   tax

setstore:
   asl

setmode:
   sta   MODE     ; set mode flags

blskip:
   iny

nextitem:
   lda   IN,y     ; get character from buffer
   cmp   #CR      ; CR -> end of line
   beq   getline
   cmp   #$ae     ; check for '.'
   bcc   blskip   ; skip everything below '.', e.g. space
   beq   setmode  ; '.' sets block XAM mode
   cmp   #$ba     ; check for ':'
   beq   setstore ; set STOR mode $
   cmp   #$d2     ; check for 'R'
   beq   run
   stx   L
   stx   H
   sty   YSAV

nexthex:
   lda   IN,y     ; get character for hex test
   eor   #$b0     ; 0-9 as text will now be $00-$09, nothing else
   cmp   #$0a     ; test for digit
   bcc   dig      ; it is a digit
   adc   #$88     ; adjust for A-F
   cmp   #$fa     ; test for hex letter
   bcc   nohex    ; it isn't a hex letter

dig:
   asl
   asl
   asl
   asl

   ldx   #$04  ; process 4 bits
hexshift:
   asl         ; move most significant bit into carry
   rol   L     ; rotate carry through 16 bit address lobyte
   rol   H     ; ...hibyte
   dex
   bne   hexshift
   iny
   bne   nexthex

nohex:
   cpy   YSAV     ; at least one digit in buffer
   beq   escape   ; no -> restart

   bit   MODE        ; check top 2 bits of mode
   bvc   notstore    ; bit 6: 0 is STOR, 1 is XAM or block XAM

; STOR mode, save LSD of new hex byte

   lda   L           ; LSD of hex data
   sta   (STL,x)     ; X=0 -> store at address in STL
   inc   STL
   bne   nextitem
   inc   STH
tonextitem:
   jmp   nextitem

;-------------------------------------------------------------------------
;  RUN user's program from last opened location
;-------------------------------------------------------------------------

run:
   jmp (XAML)        ; execute supplied address

;-------------------------------------------------------------------------
;  We're not in Store mode
;-------------------------------------------------------------------------

notstore:
   bmi   xamnext     ; bit 7: 0=XAM, 1=block XAM

; We're in XAM mode now
   ldx   #$02        ; copy 2 bytes
setaddr:
   lda   L-1,x       ; copy hex data to
   sta   STL-1,x     ; ..."store index"
   sta   XAML-1,x    ; ...and "XAM index"
   dex
   bne   setaddr

; Print address and data from this address, fall through next BNE.

nextprint:
   bne   prdata      ; no address to print
   lda   #CR
   jsr   echo        ; start new line
   lda   XAMH
   jsr   prbyte      ; print hibyte of address
   lda   XAML
   jsr   prbyte      ; print lobyte of address
   lda   #$ba        ; ":"
   jsr   echo        ; print colon

prdata:
   lda   #$a0        ; " "
   jsr   echo        ; print space
   lda   (XAML,x)    ; get data from address
   jsr   prbyte      ; print byte

xamnext:
   stx   MODE        ; set mode to XAM
   lda   XAML        ; check if
   cmp   L           ; ...there's
   lda   XAMH        ; ...more to
   sbc   H           ; ...print
   bcs   tonextitem

   inc   XAML
   bne   :+
   inc   XAMH
:
   lda   XAML
   and   #$07        ; start new line every 8 addresses
   bpl   nextprint

;-------------------------------------------------------------------------
;  Subroutine to print a byte in A in hex form (destructive)
;-------------------------------------------------------------------------

prbyte:
   pha            ; save A for LSD
   lsr            ; move MSD down to LSD
   lsr
   lsr
   lsr
   jsr   prhex    ; print MSD
   pla            ; restore A for LSD

; Fall through to print hex routine

;-------------------------------------------------------------------------
;  Subroutine to print a hexadecimal digit
;-------------------------------------------------------------------------

prhex:
   and   #$0f     ; mask LSD for hex print
   ora   #$30     ; add ascii "0"
   cmp   #$39     ; is still decimal
   bcc   echo     ; yes -> output
   adc   #$06     ; adjust offset for letters a-f

; Fall through to print routine

;-------------------------------------------------------------------------
;  Subroutine to print a character to the terminal
;-------------------------------------------------------------------------

echo:
	bit   DSP      ; test if display is able to receive char
	bmi   echo     ; loop until it does
	sta   DSP      ; write char to display
	rts

;-------------------------------------------------------------------------
;  Vector area
;-------------------------------------------------------------------------

        .byte $00,$00  ; padding

NMI_VEC:
	.word $0f00
RESET_VEC:
	.word start
IRQ_VEC:
	.word $0000
