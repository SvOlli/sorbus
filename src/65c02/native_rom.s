; IMPORTANT NOTE
; ==============
;
; This is not the real rom for the native mode. It is just a minimal
; environment used to testing purposes.
;
; The read ROM code is in another castle. (ahm, sorry, repository)

.segment "CODE"

.include "native.inc"

.define MAGIC_AT_FFF5 0

; set to 65c02 code
; ...best not make use of opcode that are not supported by 65816 CPUs
.PC02

;-------------------------------------------------------------------------
; jumptable
;-------------------------------------------------------------------------

; $E000: WozMon
RESET:
   jmp   wozmon
; $E003: run bootblock 0 from internal drive
   jmp   boot
; $E006: run bootblock id in $0f from internal drive
   jmp   bootx
; $E009: print character
   jmp   echo
; $E00C: print string
   jmp   print
; $E00F: load file
   jmp   todo
; $E012: test internal drive (temporary)
   jmp   hddtest
IRQ:
NMI:
todo:
   jmp   *

bootx:
   lda   $0f
   .byte $2c   ; skip following 2-byte instruction
boot:
   lda   #$00
   ; boottrack is 32k, memory at $E000 is 8k
   ; starting at boot+2 can load other 8k offsets from internal drive
   sei
   ldx   #$ff
   tsx
   jsr   print ; preserves all registers... spooky.
   .byte 13,10
   .byte "Sorbus Computer Native Core",13,10
   .byte "===========================",13,10
   .byte "Checking Bootsector... ",0
   ror
   ror
   ror
.if MAGIC_AT_FFF5
   ora   #$3f     ; load last sector of 8k block
.else
   and   #$c0
.endif
   sta   IDLBAL
   inx
   stx   IDLBAH
   stx   IDMEML
   ldy   #$02      ; write sector to $0200 for checking
   sty   IDMEMH

   sta   IDREAD
.if MAGIC_AT_FFF5
   and   #$c0
   sta   IDLBAL    ; reset LBA for reading to $E000
.else
   dec   IDLBAL    ; reset LBA for re-reading to $E000
.endif
   stx   IDMEML    ; adjust target memory to
   lda   #$E0      ; $E000 following
   sta   IDMEMH

   lda   IDREAD
   beq   @checksig
   jsr   print
   .byte "no internal drive",0
   bra   @errend

@notfound:
   jsr   print
   .byte "no boot signature",0
@errend:
   jsr   print
   .byte " found.",13,10,"Starting WozMon",13,10,0
   jmp   wozmon

@checksig:
   ldx   #$04
:
.if MAGIC_AT_FFF5
   lda   $0275,x
.else
   lda   $0200,x
.endif
   cmp   @signature,x
   bne   @notfound
   dex
   bpl   :-
   
   jsr   print
   .byte "found. Loading Hi-RAM from sector 00",0
   lda   IDLBAL
   jsr   prbyte
   jsr   print
   .byte ".",13,10,0

   ldy   #$40    ; banksize ($2000) / sectorsize ($80)
:
   sta   IDREAD
   dey
   bne   :-
   ; leave with Y = 0, as required in @jmpcode
   
   ldx   #$05
:
   lda   @jmpcode,x
   sta   $01fa,x
   dex
   bpl   :-
   jmp   $01fa
   
@signature:
   .byte "SBC23"
@signatureend:

@jmpcode:
   sty   BANK      ; Y = 0
   jmp   ($FFFC)
@jmpcodeend:

hddtest:
   lda   #$00
   sta   IDLBAL
   sta   IDLBAH
   sta   IDMEML
   lda   #$20 ; write sectors to $2000 following
   sta   IDMEMH
   ldx   #$10 ; read first 16 sectors to $2000-$27FF
:
   sta   IDREAD
   dex
   bne   :-

   ; X is still 0
   stx   IDLBAL
   stx   IDMEML
   lda   #$20
   sta   IDLBAH
   sta   IDMEMH

   ldx   #$10
:
   sta   IDWRT
   dex
   bne   :-

   jsr   print
   .byte 10,"SUCCESS",10,0

   jmp   wozmon




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
   jsr   echo     ; print prompt character

getline:
   lda   #LF
   jsr   echo     ; print newline

   ldy   #$01     ; start a new input, 1 compensates next line

backspace:
   lda   #$08
   jsr   echo
   dey
   bmi   getline  ; buffer underflow -> restart

nextchar:
   lda   UARTRS   ; check input
   beq   nextchar ; no input -> loop

   lda   UARTRD   ; get key
   sta   IN,y     ; add to buffer
   jsr   echo     ; print character

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
   jsr   echo        ; start new line
   lda   XAMH
   jsr   prbyte      ; print hibyte of address
   lda   XAML
   jsr   prbyte      ; print lobyte of address
   lda   #$3a        ; ":"
   jsr   echo        ; print colon

prdata:
   lda   #$20        ; " "
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
   cmp   #$3a     ; is still decimal
   bcc   echo     ; yes -> output
   adc   #$06     ; adjust offset for letters a-f

; Fall through to print routine

;-------------------------------------------------------------------------
;  Subroutine to print a character to the terminal
;-------------------------------------------------------------------------

echo:
   bit   UARTWS
   bmi   echo
   sta   UARTWR
   rts

;-------------------------------------------------------------------------
;  WozMon end
;-------------------------------------------------------------------------

print:
   sta   YSAV
   php
   pla
   sta   MODE
   pla
   sta   L
   pla
   sta   H
@loop:
   inc   L
   bne   :+
   inc   H
:
   lda   (L)
   beq   @out
   jsr   echo
   bra   @loop
@out:
   lda   H
   pha
   lda   L
   pha
   lda   MODE
   pha
   lda   YSAV
   plp
   rts

.if 0
.macro bankjsr bank,address
   php                  ; 3= 3
   sta   @BANKA         ; 4= 7
   lda   #<address      ; 2= 9
   sta   @BANKJSR+1     ; 4=13
   lda   #>address      ; 2=15
   sta   @BANKJSR+2     ; 4=19
   lda   #bank          ; 2=21
   plp                  ; 4=25
   jsr   bank_jsr_code  ; 6=31
.endmacro

bank_jsr_code:
   sta   @BANKID        ; 4=35
   php                  ; 3=38
   lda   BANK           ; 4=42
   sta   @BANKRB+1      ; 4=46
@BANKID:
   lda   #$ba           ; 2=48 ; banknumber
   sta   BANK           ; 4=52
@BANKA:
   lda   #$aa           ; 2=54 ; stored accumulator
   plp                  ; 3=57 ; get processor status from stack
@BANKJSR:
   jsr   $0000          ; 6=63 ; address
   php                  ; 3=66
   pha                  ; 3=69
@BANKRB:
   lda   #$00           ; 2=71
   sta   BANK           ; 4=75
   pla                  ; 4=79
   plp                  ; 4=83
   rts                  ; 6=89
.endif

.segment "VECTORS"
   .word NMI
   .word RESET
   .word IRQ
