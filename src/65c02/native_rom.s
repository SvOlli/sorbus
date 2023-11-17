
.segment "CODE"

UART_READ    = $DFFA; read from uart input
UART_READ_Q  = $DFFB; can be $00-$7f in normal operation
UART_WRITE   = $DFFC; write to uart output
UART_WRITE_Q = $DFFD; can be $00-$7f in normal operation

TIMER_IRQ_VALUE_REPEAT = $DF00
TIMER_IRQ_VALUE_SINGLE = $DF02
TIMER_NMI_VALUE_REPEAT = $DF04
TIMER_NMI_VALUE_SINGLE = $DF06
WATCHDOG_STOP          = $DF08
WATCHDOG_LOW           = $DF09
WATCHDOG_MID           = $DF0A
WATCHDOG_HIGH_START    = $DF0B

; set to 65c02 code
; ...best not make use of opcode that are not supported by 65816 CPUs
.PC02

IRQ:
NMI:
   jmp   *

RESET:

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

; KBD b7..b0 are inputs, b6..b0 is ASCII input, b7 is constant high
;     Programmed to respond to low to high KBD strobe
; DSP b6..b0 are outputs, b7 is input
;     CB2 goes low when data is written, returns high when CB1 goes high
; Interrupts are enabled, though not used. KBD can be jumpered to IRQ,
; whereas DSP can be jumpered to NMI.

;-------------------------------------------------------------------------
;  Constants
;-------------------------------------------------------------------------

BS              :=     $7f            ; "_": interpreted as backspace
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

wozmon:
   cld            ; just initializing bare minimum
   cli            ; note: not even stack pointer
   ldx   #$ff
   txs
   lda   #ESC     ; KBD and DSP control register mask

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
   jsr   CHROUT     ; print prompt character

getline:
   lda   #LF
   jsr   CHROUT     ; print newline

   ldy   #$01     ; start a new input, 1 compensates next line

backspace:
   lda   #$08
   jsr   CHROUT
   dey
   bmi   getline  ; buffer underflow -> restart

nextchar:
   lda   UART_READ_Q ; check input
   beq   nextchar ; no input -> loop

   lda   UART_READ      ; get key
   sta   IN,y     ; add to buffer
   jsr   CHROUT     ; print character

   cmp   #CR
   bne   notcr    ; if it's not return loop

; Line received, now let's parse it

   lda   #$00     ; default mode is XAM
   ldy   #$ff     ; reset input index
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
   ora   #$80
   cmp   #$ae     ; check for '.'
   bcc   blskip   ; skip everything below '.', e.g. space
   beq   setmode  ; '.' sets block XAM mode
   cmp   #$ba     ; check for ':'
   beq   setstore ; set STOR mode $
   and   #$5f
   cmp   #'R'     ; check for 'R'
   beq   run
   cmp   #'C'     ; check for 'R'
   beq   run_cpm
   stx   L
   stx   H
   sty   YSAV

nexthex:
   lda   IN,y     ; get character for hex test
   and   #$df     ; convert a->A, also wrecks '0'->$10
   cmp   #$10
   bcc   nohex
   cmp   #'G'     ; $41-$46
   bcs   nohex
   cmp   #$1a     ; test for digit
   bcc   dig      ; it is a digit
   adc   #$b8     ; adjust for A-F
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

run_cpm:
   jmp $080d        ; execute CPM

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
   lda   #LF
   jsr   CHROUT        ; start new line
   lda   XAMH
   jsr   prbyte      ; print hibyte of address
   lda   XAML
   jsr   prbyte      ; print lobyte of address
   lda   #$3a        ; ":"
   jsr   CHROUT        ; print colon

prdata:
   lda   #$20        ; " "
   jsr   CHROUT        ; print space
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
   cmp   #$3a     ; is still decimal
   bcc   CHROUT     ; yes -> output
   adc   #$06     ; adjust offset for letters a-f

; Fall through to print routine

;-------------------------------------------------------------------------
;  Subroutine to print a character to the terminal
;-------------------------------------------------------------------------

CHROUT:
   bit   UART_WRITE_Q
   bmi   CHROUT
   sta   UART_WRITE
	rts

GETIN:
    lda   UART_READ_Q ; check input
    bne   read_key ; no input -> return 0 
    rts

read_key:
    lda   UART_READ      ; get key
    rts

;-------------------------------------------------------------------------
;  Jumptable for accessing kernel routines
;-------------------------------------------------------------------------
.segment "TABLE"
    jmp CHROUT    ; Prints character in A to console
    jmp GETIN    ;  Reads character from console to A , returns 0 on none available


.segment "VECTORS"
   .word NMI
   .word RESET
   .word IRQ
