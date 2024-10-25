;        ------- Instant 6502 Assembler for the KIM-1 --------
; ** coded for the acme cross assembler **
; The assembler accepts all valid 6502 instructions and operands
; and places the assembled instructions into memory.
;
; The assembler is presented more for curiosity value than as a serious
; programming tool. It was written for the base model KIM-1 with very
; little memory, to ease the task of entering programs.
; It can occupy less than 340 bytes.
; (The version here is 354 bytes, 17 are unused).
; Due to the small footprint, there is no error checking. It is up to
; the programmer to ensure only valid instructions are entered.
;
; All alphabetics are entered in upper case only. All data (addresses,
; immediate etc) is entered as hex digits. One useful facility is branch
; instructions take the absolute address and calculate the relative address.
;
; Some input is accepted as soon as the last character is input and
; does not require a terminating character. This applies to implied
; instructions (that have no operand) and byte input (#hh). E.g. if
; TAX is input, as soon as the X is input the assembler generates the
; code and prompts for the next instruction
;
; Where an operand is required, the input of the operand is completed
; by entering a character other than one of [hex , # ( ) X Y]. Conventionally
; that character is a space. Control characters are best avoided.
;
; operands take the form (where h is a hexadecimal digit)
; A - accumulator (for ROL etc.)   hh - zero page address  #hh - immediate
; hh,X  hh,Y - zero page indexed   hhhh - absolute address
; hhhh,X  hhhh,Y - absolute indexed  (hh,X)  (hh),Y - indexed indirect
; (hhhh) - absolute indirect NOTE: branches require an absolute address
;
; When the assembler is started, it is usual to enter an address where
; code will be placed otherwise the assembler will overwrite itself with
; unpredictable reults.
;
; An entry address is input as *hhhh<sp>. This can be input at any time.
;
; Other valid input:
; #hh generates the byte hh
; < cancels the current input (for instance, if a mistake is made)
; / returns to the KIM monitor
;

.define SORBUS 1 ; set this to 0 to build KIM-1 version

.define CPUID  $DF04
.define CHRIN  $FF00
.define CHROUT $FF03
.define PRINT  $FF06

.segment "CODE"
;      *= $0200      ; set program counter
;      !to "org0200e.bin", plain   ; set output file and format
.if SORBUS
start:
   jmp   message

   INL    = $F8
   INH    = $F9
   POINTL = $FA
   POINTH = $FB

open:
   lda   INL
   sta   POINTL
   lda   INH
   sta   POINTH
   rts

crlf:
   lda   #$0a
   jmp   CHROUT

outsp:
   lda   #$20
   jmp   CHROUT

prtpnt:
   lda   POINTH
   jsr   prtbyt
   lda   POINTL
prtbyt:
   pha
   lsr
   lsr
   lsr
   lsr
   jsr   prthex
   pla
   pha
   jsr   prthex
   pla
   rts

prthex:
   and   #$0f
   ora   #'0'
   cmp   #':'
   bcc   :+
   adc   #$06
:
   jmp   CHROUT


getch:
   jsr   CHRIN
   bcs   getch

   cmp   #'a'
   bcc   :+
   cmp   #'z'+1
   bcs   :+
   and   #$df        ; uppercase
:

   cmp   #$0d
   bne   :+
   lda   #' '        ; convenience: return -> space, so return ends input
:
   cmp   #$7f
   bne   :+
   lda   #'<'        ; convenience: backspace -> "<", to cancel input
:
   jmp   CHROUT

getbyt:
   jsr   getch
   jsr   pack
   jsr   getch
   jsr   pack
   lda   INL
   rts

incpt:
   inc   POINTL
   bne   :+
   inc   POINTH
:
   rts

pack:
   cmp   #'0'
   bmi   @done

   cmp   #'F'+1
   bpl   @done

   cmp   #'@'
   bmi   @decimal

   clc
   adc   #$09
@decimal:
   rol
   rol
   rol
   rol

   ldy   #$04
:
   rol
   rol   INL
   rol   INH
   dey
   bne   :-

   lda   #$00
@done:
   rts

exit:
   jmp   ($fffc)

.else
; define some KIM-1 ROM addresses
   open    = $1fcc ; copy INL/INH to POINTL/POINTH
   crlf    = $1e2f ; print out $0d,$0a
   prtpnt  = $1e1e ; print out address in POINTL/POINTH
   outsp   = $1e9e ; print out space
   getch   = $1e5a ; print out char in A
   getbyt  = $1f9d ; get to hex chars from input into INL/INH
   prtbyt  = $1e3b ; print out hex byte
   incpt   = $1f63 ; increment POINTL/POINTH
   pack    = $1fac ; input hex, if is (INL/INH) << 4 | input
   exit    = $1c16

   jmp   x40b   ; convenience, start address same as load point
.endif
; data area
;   membyt is the number of bytes to be put into memory when the instruction is encoded
;   it is cleared to zero early so #hh data items are limited to 1 byte of output.
;   For any instruction or a #hh data item, one byte is always put into memory.
;   Any extra bytes are determined by the number of hex characters in the operand.
;   The number of bytes is (N + no. of hex)/2-1. N=2 for most instructions.
;   N=0 for branch instructions because the 4 hex characters absolute address is reduced
;   to a one byte relative address
   membyt  = $f5
   charin  = $f6

; ** program **

; if the first character is '*' - input 4 hex digits and set this as
; the current address
x403:
   jsr   x4de   ; gets a character and calls pack
   beq   x403   ; A=0 it was a hex character, try for another
   jsr   open   ; not a hex character, set current location to INL,INH
; next instruction
x40b:
   ldx   #$ff   ; set the stack empty
   txs
; CR LF print the current location and 3 spaces
   jsr   crlf   ; next line
   jsr   prtpnt   ; the current location
   ldx   #$03   ; three spaces
   jsr   x4e6
   stx   membyt   ; x=0 on return, initial count of bytes to place in memory
   ldx   #$03
; f6 is the count of characters input and echoed including the instruction. When accepting the
; instruction or #hh the characters are not counted as they are input. So start the count at 3
   stx   charin   ; count of characters input
; also use X to count 3 input characters
x41f:
   jsr   x4d6   ; get a character
   cmp   #'/'
   bne   x429
   jmp   exit    ; if /, return to monitor
x429:
   cmp   #'*'
   beq   x403   ; if *, get a new location
   cmp   #'#'
   bne   x437   ; if #, a data byte
   jsr   getbyt
   jmp   x4a5
; assume it's an instruction - expect 3 letters A thru Y
x437:
   tay
   lda   x500-'A',y ; lookup pattern for character
   and   x55f-1,x ; apply mask for 1st,2nd or 3rd character of mnemonic
   sta   $f6,x ; save the masked value into $f9,$f8
   dex
   bne   x41f
; value for character 3 is xx000000. Adjust to 000000xx
   asl   ; a - got 3 characters, create index into instructions
   rol   ; a
   rol   ; a
; add in the other two masked values
   adc   $f8
   adc   $f9
   tax
   lda   x519,x ; get the basic instruction code
   sta   $f7
; x = 2 for different reasons
   ldx   #$02
; if implied, becomes X = 1 for instruction modifier lookup
; if not implied, 2 spaces before accepting the operand
   and   #$05 ; work out what type
; the basic instruction code has the form xxxxxAxB
; AB = 00, branch : AB = 01, implied : AB = 1x, other
   lsr   ; a
   sta   $f4 ; instruction modifier lookup
   sta   membyt ; bytes to output
   bne   *+4
   bcs   x49f ; an implied - go to output
   pha
   jsr   x4e6 ; output 2 spaces
x461:
   jsr   x4de ; get a character
   bne   *+4 ; skip if not hex
   inc   membyt ; bytes to put in memory
   ldx   #$07 ; search table for matching operand character
x46a:
   cmp   x557,x ; possible operand components (X, Y, brackets etc)
   bne   x477
; the index is manipulated to yield:
; , -> 0 (can be left out) ; # or a hex digit -> 1 ; ( -> 2 |  ) or X -> 3 ; Y -> 4
   txa
   lsr   ; a
   adc   $f4 ; add to modifier lookup
   sta   $f4
   bne   x461 ; should always branch, get next component
x477:
   dex
   bpl   x46a ; didn't match, try the next one
; drop thru if not a hex digit or operand component
   pla
   bne   x487 ; 0 = branch instruction, 2 otherwise
; convert absolute to relative address for branches
   lda   $f8
   sec
   sbc   #$02
   sec
   sbc   $fa
   sta   $f8
; some instructions are not consistent - special processing
x487:
   ldx   $f4
   lda   $f7
   cmp   #$34
   bne   *+4
   ldx   #$0d
   and   #$08
   beq   x4a0
   cpx   #$0a
   beq   x49f
   cpx   #$05
   bne   x4a0
   dex
   dex
x49f:
   dex
; modify base code according to address mode
x4a0:
   eor   x4c8-1,x
   eor   $f7
x4a5:
   sta   $f7 ; the final op code
; pad out with spaces so the generated byte display
; lines up
   lda   charin
   eor   #$0f   ; effectively a 1s complement subtract from 15
   tax
   jsr   x4e6
   lsr   membyt ; membyt = membyt/2
x4b1:
   jsr   outsp  ; a space
   lda   $f7,x  ; a byte to put in memory
   jsr   prtbyt ; display it
   ldy   #$00   ; put it in memory
   sta   ($fa),y
   jsr   incpt  ; increment the memory pointer
   inx
   cpx   membyt ; all output?
   bmi   x4b1  ; no, go back and do the next one
x4c5:
   jmp   x40b  ; yes, next instruction
;
; op code adjustment table
x4c8:
   .byte $01, $04, $0c, $00, $0c, $08, $10, $10
   .byte $18, $1c, $28, $04, $14
   .byte $00 ; not used ??

x4d6:
   jsr   getch
   cmp   #'<'  ; the cancel character
   beq   x4c5 ; restarts the input
   rts
; get a character and try to pack it as hex
; on return A=0 if it was hex, A=character otherwise
x4de:
   jsr   x4d6
   inc   charin
   jmp   pack

x4e6:
   inc   charin ; output X spaces
   jsr   outsp
   dex
   bne   x4e6
   rts
;
.if SORBUS
.else
   .res 17
.endif
; character lookup table -
x500:
   ;      A    B    C    D    E    F    G    H
   .byte $32, $4b, $60, $97, $77, $00, $00, $00
   ;      I    J    K    L    M    N    O    P
   .byte $1e, $00, $40, $1c, $00, $3a, $11, $d6
   ;      Q    R    S    T    U    V    W    X
   .byte $c0, $7e, $ad, $c3, $00, $c3, $00, $80
   ;      Y
   .byte $c1
;
; base instruction patterns
x519:
   .byte $8b, $99, $9b, $44, $ab, $a9, $34, $bb
   .byte $30, $90, $b0, $d0, $50, $70, $10, $01
   .byte $49, $24, $f0, $09, $69, $00, $05, $29
   .byte $c6, $cb, $89, $e6, $e9, $c9, $46, $a5
   .byte $00, $ae, $ac, $c5, $59, $19, $d9, $b9
   .byte $ec, $cc, $00, $85, $e5, $86, $84, $79
   .byte $39, $f9, $45, $00, $25, $06, $00, $00
   .byte $65, $26, $66, $41, $eb, $61
; lookup operand characters
x557:   .byte ',', '#', 0, '(', $ff, ')', 'X', 'Y'
; masks used on data retrieved from character lookup
x55f:   .byte $c0, $07, $38

.if SORBUS
message:
   ; message will be shown only on first start
   ; by changing start address to original
   lda   #<message
   sta   POINTL
   lda   #>message
   sta   POINTH
   lda   #<x40b
   sta   start+1
   lda   #>x40b
   sta   start+2

   lda   CPUID
   lsr            ; is this NMOS?
   bcc   cmos
   ldx   #$00
:
   lda   welcome,x
   beq   :+
   jsr   CHROUT
   inx
   bne   :-
:
   jmp   x40b     ; skip intoductions
cmos:

   jsr   PRINT ;345678901234567890123456789012345678901234567890123456789012345678901234567890
welcome:
   .byte $0a,"Instant 6502 Assembler for the KIM-1 ported to the Sorbus Computer"
   .byte $0a,"written by Alan Cashin"
   .byte $0a,$00
   jsr   PRINT
   .byte $0a,"The assembler is presented more for curiosity value than as a serious"
   .byte $0a,"programming tool. It was written for the base model KIM-1 with very little"
   .byte $0a,"memory, to ease the task of entering programs."
   .byte $0a
   .byte $0a,"Due to the small footprint, there is no error checking. It is up to the"
   .byte $0a,"programmer to ensure only valid instructions are entered."
   .byte $0a
   .byte $0a,"All alphabetics are entered in upper case only. All data (addresses,immediate"
   .byte $0a,"etc) is entered as hex digits. One useful facility is branch instructions take"
   .byte $0a,"the absolute address and calculate the relative address."
   .byte $0a
   .byte $0a,"Some input is accepted as soon as the last character is input and does not"
   .byte $0a,"require a terminating character. This applies to implied instructions (that"
   .byte $0a,"have no operand) and byte input (#hh). E.g. if TAX is input, as soon as the X"
   .byte $0a,"is input the assembler generates the code and prompts for the next instruction."
   .byte $0a
   .byte $0a,"Where an operand is required, the input of the operand is completed by entering"
   .byte $0a,"a character other than one of [hex , # ( ) X Y]. Conventionally that character"
   .byte $0a,"is a space. Control characters are best avoided."
   .byte $0a
   .byte $0a,"Press SPACE "
   .byte $00
:
   jsr   getch
   cmp   #' '
   bne   :-

   jsr   PRINT
   .byte $0d,"operands take the form (where h is a hexadecimal digit):"
   .byte $0a,"A - accumulator (for ROL etc.)   hh - zero page address  #hh - immediate"
   .byte $0a,"hh,X  hh,Y - zero page indexed   hhhh - absolute address"
   .byte $0a,"hhhh,X  hhhh,Y - absolute indexed  (hh,X)  (hh),Y - indexed indirect"
   .byte $0a,"(hhhh) - absolute indirect NOTE: branches require an absolute address"
   .byte $0a
   .byte $0a,"An entry address is input as *hhhh<sp>. This can be input at any time."
   .byte $0a
   .byte $0a,"Other valid input:"
   .byte $0a,"#hh generates the byte hh"
   .byte $0a,"< cancels the current input (for instance, if a mistake is made)"
   .byte $0a,"/ exits the assembler"
   .byte $0a
   .byte $0a,"It is suggested to start the entered program using"
   .byte $0a,"WozMon's ",$22,"R",$22," command. Assembler can be restarted with ",$22,"0400 R",$22
   .byte $0a,"On restart, this message will not be displayed"
   .byte $00
   jsr   crlf
   jmp   x40b
.endif

