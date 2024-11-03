
.include "../native.inc"
.include "../native_bios.inc"

; TODOs:
; [X] remove bit 7 when using "'" to input text
; [X] strip off machine state from registers
; [X] GO read registers and sets after RTS
; [X] load and save functions
;     (TODO: check why C code locks up when start and end are swapped)
; [X] memory show uses 16 instead of 8 bytes
; [ ] memory show also displays ASCII
; [ ] BRK with parameter
; [X] switch input to "int lineimput"
; [X] remove unnecessary code
; [ ] support bankswitch $DF00


;******************************************************************************
;* Apple //c System Monitor                                                   *
;* ported to the Sorbus Computer                                              *
;* in 2024 by "SvOlli" Sven Oliver Moll                                       *
;******************************************************************************
;* parts taken from:                                                          *
;******************************************************************************
;* Apple IIc ROM3 (slinky)  - Bank 1                                          *
;* Re-created by James Lewis (@baldengineer)                                  *
;* ******************************************************************         *
;*  2023-03-05 v0.9: Decode of bank 1 done                                    *
;*                   Needs lots of proof-reading!                             *
;*                     Skipped AppleSoft ($D000-$F7FF).                       *
;*                     Left ROM Switch Rountines ($C780) un-decoded           *
;*                      (SourceGen doesn't like the bank 2 addrs)             *
;* ******************************************************************         *
;*                                                                            *
;* For more information visit:                                                *
;* https://github.com/baldengineer/Apple-IIc-ROM-Disassembly                  *
;*                                                                            *
;* (Original attributions are in comments below, as shown in original ROM     *
;* Listings.)                                                                 *
;*                                                                            *
;* Recreated work licensed as Attribution-NonCommercial-ShareAlike 4.0        *
;* International.                                                             *
;*                                                                            *
;* Generated with 6502bench SourceGen v1.8.4                                  *
;******************************************************************************
;* based upon:                                                                *
;******************************************************************************
;*                                                                            *
;*  Apple //c                                                                 *
;*  Video Firmware and                                                        *
;*  Monitor ROM Source                                                        *
;*                                                                            *
;*  COPYRIGHT 1977-1983 BY                                                    *
;*  APPLE COMPUTER, INC.                                                      *
;*                                                                            *
;*  ALL RIGHTS RESERVED                                                       *
;*                                                                            *
;*  S. WOZNIAK           1977                                                 *
;*  A. BAUM              1977                                                 *
;*  JOHN A           NOV 1978                                                 *
;*  R. AURICCHIO     SEP 1982                                                 *
;*  E. BEERNINK          1983                                                 *
;*                                                                            *
;******************************************************************************

; 65SC02 is minimum requirement
;.psc02

.define CONFIG_CMD_NOT_FOUND  0
.define CONFIG_SHOW_HELP      1

.if 1
zpstart         :=     $e6

LMNEM           :=     $e6    ;{addr/1}   ;temp for mnemonic decoding
RMNEM           :=     $e7    ;{addr/1}   ;temp for mnemonic decoding
FORMAT          :=     $e8    ;{addr/1}   ;temp for opcode decode
LENGTH          :=     $e9    ;{addr/1}   ;temp for opcode decode

SCRWIDTH        :=     $ea    ;{addr/1}   ;storing screen with for input line
PROMPT          :=     $eb    ;{addr/1}   ;prompt characater
YSAV            :=     $ec    ;{addr/1}   ;position in Monitor command
YSAV1           :=     $ed    ;{addr/1}   ;temp for Y register

PCL             :=     $ee    ;{addr/1}   ;temp for program counter
PCH             :=     $ef    ;{addr/1}
A1L             :=     $f0    ;{addr/1}   ;Monitor Temp
A1H             :=     $f1    ;{addr/1}   ;Monitor Temp
A2L             :=     $f2    ;{addr/1}   ;Monitor Temp
A2H             :=     $f3    ;{addr/1}   ;Monitor Temp
A3L             :=     $f4    ;{addr/1}   ;Monitor Temp
A3H             :=     $f5    ;{addr/1}   ;Monitor Temp
A4L             :=     $f6    ;{addr/1}   ;Monitor Temp
A4H             :=     $f7    ;{addr/1}   ;Monitor Temp
A5L             :=     $f8    ;{addr/1}   ;Monitor Temp
A5H             :=     $f9    ;{addr/1}   ;Monitor Temp
MODE            :=     $fa    ;{addr/1}   ;Monitor mode
ACC             :=     $fb    ;{addr/1}   ;Acc after BRK
XREG            :=     $fc    ;{addr/1}   ;X reg after break
YREG            :=     $fd    ;{addr/1}   ;Y reg after break
STATUS          :=     $fe    ;{addr/1}   ;P reg after break
SPNT            :=     $ff    ;{addr/1}   ;SP After Break
.else
; old positions for historical reasons, will be removed when stable!
LMNEM           :=     $2C    ;{addr/1}   ;temp for mnemonic decoding
RMNEM           :=     $2D    ;{addr/1}   ;temp for mnemonic decoding
FORMAT          :=     $2E    ;{addr/1}   ;temp for opcode decode
LENGTH          :=     $2F    ;{addr/1}   ;temp for opcode decode

MODE            :=     $31    ;{addr/1}   ;Monitor mode

PROMPT          :=     $33    ;{addr/1}   ;prompt characater
YSAV            :=     $34    ;{addr/1}   ;position in Monitor command
YSAV1           :=     $35    ;{addr/1}   ;temp for Y register
CSWL            :=     $36    ;{addr/1}   ;character output hook
CSWH            :=     $37    ;{addr/1}

PCL             :=     $3A    ;{addr/1}   ;temp for program counter
PCH             :=     $3B    ;{addr/1}
A1L             :=     $3C    ;{addr/1}   ;Monitor Temp
A1H             :=     $3D    ;{addr/1}   ;Monitor Temp
A2L             :=     $3E    ;{addr/1}   ;Monitor Temp
A2H             :=     $3F    ;{addr/1}   ;Monitor Temp
A3L             :=     $40    ;{addr/1}   ;Monitor Temp
A3H             :=     $41    ;{addr/1}   ;Monitor Temp
A4L             :=     $42    ;{addr/1}   ;Monitor Temp
A4H             :=     $43    ;{addr/1}   ;Monitor Temp
A5L             :=     $44    ;{addr/1}   ;Monitor Temp
MSTATE          :=     $44    ;{addr/1}   ;Monitor Temp
A5H             :=     $45    ;{addr/1}   ;Monitor Temp
ACC             :=     $45    ;{addr/1}   ;Acc after BRK
XREG            :=     $46    ;{addr/1}   ;X reg after break
YREG            :=     $47    ;{addr/1}   ;Y reg after break
STATUS          :=     $48    ;{addr/1}   ;P reg after break
SPNT            :=     $49    ;{addr/1}   ;SP After Break
.endif
IN              :=     $0200  ;{addr/2}   ;input buffer for GETLN


; SX4 always starts at load address
wozmon2c:
   php
   ldx   #zpstart
:
   stz   $00,x
   inx
   bne   :-
   pla                     ;SAVE STATUS FOR DEBUG SOFTWARE
   sta   STATUS

.if CONFIG_SHOW_HELP
   jsr   PRINT
   .byte 10,"WozMon port based on Apple //c"
   .byte 10,"Supported commands:"
   .byte 10,"show:   start.end"
   .byte 10,"change: start: value value value"
   .byte 10,"verify: dest<start.endV"
   .byte 10,"move:   dest<start.endM"
   .byte 10,"disass: startL"
   .byte 10,"go:     startG"
   .byte 10,"assemb: !"
   .byte 10,"regs:   R"
   .byte 10,"dir:    user$"
   .byte 10,"load:   user<start{filename.ext"
   .byte 10,"save:   user<start.end}filename.ext"
   .byte 10,"quit:   X"
   .byte 10
   .byte 10,"More detailed instructions at:"
   .byte 10,"https://github.com/SvOlli/sorbus/blob/master/doc/monitors.md"
   .byte 10,0
.endif

   tsx
   stx   SPNT

   ldy   #VT100_SCRN_SIZ   ; get screen size for line input size
   int   VT100

   cpx   #$82              ; if width >= 130 characters
   bcc   :+
   ldx   #$81              ; stick to 127 in the end (see below)
:
   dex                     ; subtract one for prompt
   dex                     ; subtract one for cursor at end of line
   stx   SCRWIDTH

   jmp   MON

EXIT:
   jmp   ($FFFC)

DIR:
.if 0
   jsr   PRINT
   .byte "User:",0
.endif
   stz   CPM_SADDR+1
   lda   A2L
   and   #$0f
   tay
.if 0
   int   PRHEX8
   lda   #$0a
   jsr   CHROUT
.endif
   int   CPMDIR
   rts

ioerrorsub:
   pla
   pla
ioerror:
   jsr   PRINT
   .byte "i/o error",0
   rts

setfilename:
   lda   A2L
   sta   CPM_EADDR+0
   lda   A2H
   sta   CPM_EADDR+1
   ; adjust end address, CPMSAVE expects it in "CBM format" (addr+1)
   inc   CPM_EADDR+0
   bne   :+
   inc   CPM_EADDR+1
:
   lda   A1L
   sta   CPM_SADDR+0
   lda   A1H
   sta   CPM_SADDR+1
   cmp   #$04
   bcc   ioerrorsub

   ldy   YSAV
   phy
   tya
   dey
   lda   #$8d
   sta   IN,y
   dec   YSAV
:
   iny
   lda   IN,y
   cmp   #$8d              ; end of input?
   beq   @done
   and   #$7f
   sta   IN,y
   bra   :-

@done:
   lda   #$00
   sta   IN,y

   lda   A4L
   and   #$0f
   tay
   pla
.if .lobyte(IN)
.error IN must be at page start
.endif
   ldx   #>IN
   int   CPMNAME
   rts


LOAD:
   jsr   setfilename
   int   CPMLOAD
   bcs   ioerror
   rts

SAVE:
   jsr   setfilename
   int   CPMSAVE
   bcs   ioerror
   rts

;******************************************************************************
;* Sorbus subroutines end here                                                *
;******************************************************************************


;******************************************************************************
;
; Apple //c Mini Assembler
;
; Got mnemonic, check address mode
;
AMOD1:
   jsr   NNBL              ;get next non-blank
   sty   YSAV              ;save Y
   cmp   CHAR1,x
   bne   AMOD2
   jsr   NNBL              ;get next non-blank
   cmp   CHAR2,x
   beq   AMOD3
   lda   CHAR2,x           ;done yet?
   beq   AMOD4
   cmp   #$A4              ;if "$" then done
   beq   AMOD4
   ldy   YSAV              ;restore Y
AMOD2:
   clc
AMOD4:
   dey
AMOD3:
   rol   A5L               ;shift bit into format
   cpx   #$03
   bne   AMOD6
   jsr   GETNUM
   lda   A2H               ;get high byte of address
   beq   AMOD5             ;=>
   inx
AMOD5:
   stx   YSAV1
   ldx   #$03
   dey
AMOD6:
   stx   A1H
   dex
   bpl   AMOD1
   rts

;
;
; calculate offset byte for relative addresses
;
REL:
   sbc   #$81              ;calc relative address
   lsr
   bne   GOERR             ;bad branch
   ldy   A2H
   ldx   A2L
   bne   REL1
   dey   ;point to offset
REL1:
   dex   ;displacement -1
   txa
   clc
   sbc   PCL               ;subtract current PCL
   sta   A2L               ;and save as displacement
   bpl   REL2              ;check page
   iny
REL2:
   tya   ;get page
   sbc   PCH               ;check page
GOERR:
   bne   MINIERR           ;display error
;
; Move instruction to memory
;
MOVINST:
   ldy   LENGTH            ;get instruction length
MOV1:
   lda   A1H,y             ;get a byte
   sta   (PCL),y
   dey
   bpl   MOV1
;
; Display information
;
   jsr   PRBLNK            ;print blanks to make ProDOS work
   ldy   #VT100_CPOS_GET
   int   VT100
   dec                     ;move up 2 lines
   dec
   ldy   #VT100_CPOS_SET
   int   VT100
   jsr   showinst          ;Display line & get next instruction
; Get the next instruction
GETINST1:
   lda   #$A1              ;! for prompt
   sta   PROMPT
   jsr   GETLNZ            ;Get a line
   bra   DOINST            ;Go do the instruction

;
; Compare disassembly of all known opcodes
; with the one types in until a match is found
;
GETOP:
   lda   A1H               ;get opcode
   jsr   INDS2             ;determine mnemonic index
   asl
   tax
   lda   MNEM+0,x
   cmp   A4L
   bne   NXTOP
   lda   MNEM+1,x
   cmp   A4H               ;does it match entry?
   bne   NXTOP             ;=>no, try next opcode
   lda   A5L               ;found opcode, check address mode
   ldy   FORMAT            ;get addr. mode format for that opcode
   cpy   #$9D              ;is it relative?
   beq   REL               ;=>yes, calc relative address
   cmp   FORMAT            ;does mode match?
   beq   MOVINST           ;=>yes, move instruction to memory
NXTOP:
   dec   A1H               ;else try next opcode
   bne   GETOP             ;=>go try it
   inc   A5L               ;else try next format
   dec   YSAV1
   beq   GETOP             ;=>go try next foramt
;
; Point to the error with a caret, beep, and fall
; into the mini-assembler.
;
MINIERR:
   ldy   YSAV              ;get position
ERR2:
   tya
   tax
   jsr   PRBL2
   lda   #$DE              ;^ to point to error
   jsr   cout
   bra   GETINST1          ;try again

;
; Read a line of input.  If prefaced with " ", decode
; mnemonic. If "$" do monitor command.  Otherwise parse
; hex address before decoding mnemonic.
;
DOINST:
   jsr   ZMODE             ;clear mode (set Y=$00)
   lda   IN                ;get first char in line
   cmp   #$A0              ;if blank,
   beq   DOLIN             ;=>go attempt disassembly
   cmp   #$8D              ;is it return?
   bne   GETI1             ;=>no, continue
   rts                     ;else return to Monitor

GETI1:
   jsr   GETNUM            ;prase hexadecimal input
   cmp   #$93              ;look for "ADDR:"
GOERR2:
   bne   ERR2              ;no ":", display error
   txa                     ;X non zero if address entered
   beq   ERR2              ;no "ADDR", display error
;
   jsr   A1PCLP            ;move address to PC
DOLIN:
   lda   #$03              ;get starting opcode
   sta   A1H               ;and save
NXTCH:
   jsr   NNBL              ;get next non-blank
   asl
   sbc   #$BE
   cmp   #$C2
   bcc   ERR2              ;=>flag bad mnemonic
;
; Form mnemonic for later comparison
;
   asl
   asl
   ldx   #$04
NXTMN:
   asl
   rol   A4L
   rol   A4H
   dex
   bpl   NXTMN
   dec   A1H               ;decrement mnemonic count
   beq   NXTMN
   bpl   NXTCH
   ldx   #5                ;index into address mode tables
   jsr   AMOD1             ;do this elsewhere
   lda   A5L               ;get format
   asl
   asl
   ora   YSAV1
   cmp   #$20
   bcs   AMOD7
   ldx   YSAV1             ;get our format
   beq   AMOD7
   ora   #$80
AMOD7:
   sta   A5L               ;update format
   sty   YSAV              ;update position
   lda   $0200,y           ;get next character
   cmp   #$BB              ;is it a ";"?
   beq   AMOD8             ;=>yes, skip comment
   cmp   #$8D              ;is it a carriage return
   bne   GOERR2
AMOD8:
   jmp   GETOP             ;get next opcode

;********************************************************************************
;*                                                                              *
;* NNBL - Gets a non blank character for the mini assembler                     *
;*                                                                              *
;********************************************************************************
NNBL:
   jsr   GETUP             ;Get next upshifted character
   cmp   #$A0              ;Blank?
   beq   NNBL
   rts


;******************************************************************************
; NEWOPS translates the opcode in the Y register
; to a mnemonic table index and returns with Z=1.
; If Y is not a new opcode, Z=0.
;
NEWOPS:
   tya   ;get the opcode
   ldx   #<(INDX-OPTBL-1)  ;check through new opcodes
NEWOP1:
   cmp   OPTBL,x           ;does it match?
   beq   GETINDX           ;=>yes, get new index
   dex
   bpl   NEWOP1            ;else check next one
   rts   ;not found, exit with BNE

GETINDX:
   lda   INDX,x            ;lookup index for mnemonic
   ldy   #0                ;exit with BEQ
   rts


;******************************************************************************
SCRN2:
   bcc   RTMSKZ            ;IF EVEN, USE LO H
   lsr
   lsr
   lsr   ;SHIFT HIGH HALF BYTE DOWN
   lsr
RTMSKZ:
   and   #$0F              ;MASK 4-BITS
   rts


;******************************************************************************
INSDS1:
   ldx   PCL               ;PRINT PCL,H
   ldy   PCH
   jsr   PRYX2
   jsr   PRBLNK            ;FOLLOWED BY A BLANK (will set X=$00)
   ;lda   (PCL,x)           ;GET OPCODE
   lda   (PCL)             ;GET OPCODE
INDS2:
   tay   ;LABLE moved down 1
   lsr                     ;EVEN/ODD TEST
   bcc   IEVEN
   ror                     ;BIT 1 TEST
   bcs   ERR               ;XXXXXX11 INVALID OP
   and   #$87              ;MASK BITS
IEVEN:
   lsr                     ;LASB INTO CARRY FOR L/R TEST
   tax
   lda   FMT1,x            ;GET FORMAT INDEX BYTE

   jsr   SCRN2             ;R/L H-BYTE ON CARRY
   bne   GETFMT
ERR:
   ldy   #$FC              ;SBSTITUTE $FC FOR INVALID OPS
   lda   #$00              ;SET PRINT FORMAT INDEX TO 0
GETFMT:
   tax
   lda   FMT2,x            ;INDEX INTO PRINT FORMAT TABLE
   sta   FORMAT            ;SAVE FOR ADR FIELD FORMATTING
   and   #$03              ;MASK FOR 2-BIT LENGTH
; (0=1 BYTE, 1=2 BYTE, 2=3 BYTE)
   sta   LENGTH
   jsr   NEWOPS            ;get index for new opcodes
   beq   GOTONE            ;found a new op (or no op)
   and   #$8F              ;MASK FOR 1XXX1010 TEST
   tax   ; SAVE IT
   tya   ;OPCODE TO A AGAIN
   ldy   #$03
   cpx   #$8A
   beq   MNNDX3
MNNDX1:
   lsr
   bcc   MNNDX3            ;FORM INDEX INTO MNEMONIC TABLE
   lsr
MNNDX2:
   lsr                     ;  1) 1XXX1010 => 00101XXX
   ora   #$20              ;  2) XXXYYY01 => 00111XXX
   dey   ;  3) XXXYYY10 => 00110XXX
   bne   MNNDX2            ;  4) XXXYY100 => 00100XXX
   iny   ;  5) XXXXX000 => 000XXXXX
MNNDX3:
   dey
   bne   MNNDX1
GOTONE:
   rts


;******************************************************************************
instdsp:
   jsr   INSDS1            ;GEN FMT, LEN BYTES (sets Y=$00)
   pha   ;SAVE MNEMONIC TABLE INDEX
PRNTOP:
   ;lda   (PCL),y
   lda   (PCL)
   jsr   PRBYTE
   ldx   #$01              ;PRINT 2 BLANKS
PRNTBL:
   jsr   PRBL2
   cpy   LENGTH            ;PRINT INST (1-3 BYTES)
   iny   ;IN A 12 CHR FIELD
   bcc   PRNTOP
   ldx   #$03              ;CHAR COUNT FOR MNEMONIC INDEX
   cpy   #$04
   bcc   PRNTBL
   pla   ;RECOVER MNEMONIC INDEX
   asl
   tay
   lda   MNEM+1,y
   sta   LMNEM             ;FETCH 3-CHAR MNEMONIC
   lda   MNEM+0,y          ;  (PACKED INTO 2-BYTES)
   sta   RMNEM
PRMN1:
   lda   #$00
   ldy   #$05
PRMN2:
   asl   RMNEM             ;SHIFT 5 BITS OF CHARACTER INTO A
   rol   LMNEM
   rol                     ;  (CLEARS CARRY)
   dey
   bne   PRMN2
   adc   #$BF              ;ADD "?" OFFSET
   jsr   cout              ;OUTPUT A CHAR OF MNEM
   dex
   bne   PRMN1
   jsr   PRBLNK            ;OUTPUT 3 BLANKS
   ldy   LENGTH
   ldx   #$06              ;CNT FOR 6 FORMAT BITS
PRADR1:
   cpx   #$03
   beq   PRADR5            ;IF X=3 THEN ADDR.
PRADR2:
   asl   FORMAT
   bcc   PRADR3
   lda   CHAR1-1,x
   jsr   cout
   lda   CHAR2-1,x
   beq   PRADR3
   jsr   cout
PRADR3:
   dex
   bne   PRADR1
   rts

PRADR4:
   dey
   bmi   PRADR2
   jsr   PRBYTE
PRADR5:
   lda   FORMAT
   cmp   #$E8              ;HANDLE REL ADR MODE
   lda   (PCL),y           ;SPECIAL (PRINT TARGET,
   bcc   PRADR4            ;  NOT OFFSET)
   jsr   pcadj3
   tax                     ;PCL,PCH+OFFSET+1 TO A,Y
   inx
   bne   PRNTYX            ;+1 TO Y,X
   iny
PRNTYX:
   tya
PRNTAX:
   jsr   PRBYTE            ;OUTPUT TARGET ADR
PRNTX:
   txa   ;  OF BRANCH AND RETURN
   jmp   PRBYTE

PRBLNK:
   ldx   #$03              ;BLANK COUNT
PRBL2:
   lda   #$A0              ;LOAD A SPACE
   jsr   cout              ;OUTPUT A BLANK
   dex
   bne   PRBL2             ;LOOP UNTIL COUNT=0
   rts

pcadj:
   sec   ;0=1 BYTE, 1=2 BYTE,
   lda   LENGTH            ;  2=3 BYTE
pcadj3:
   ldy   PCH
   tax   ;TEST DISPLACEMENT SIGN
   bpl   pcadj4            ;  (FOR REL BRANCH)
   dey   ;EXTEND NEG BY DECR PCH
pcadj4:
   adc   PCL
   bcc   RTS2              ;PCL+LENGTH(OR DISPL)+1 TO A
   iny   ;  CARRY INTO Y (PCH)
RTS2:
   rts


;******************************************************************************
NXTA4:
   inc   A4L               ;INCR 2-BYTE A4
   bne   NXTA1             ; AND A1
   inc   A4H
NXTA1:
   lda   A1L               ;INCR 2-BYTE A1.
   cmp   A2L               ; AND COMPARE TO A2
   lda   A1H               ; (CARRY SET IF >=)
   sbc   A2H
   inc   A1L
   bne   RTS4B
   inc   A1H
RTS4B:
   rts


;******************************************************************************
;* Move / Verify                                                              *
;******************************************************************************
LT:
   ldx   #$01
LT2:
   lda   A2L,x             ;COPY A2 (2 BYTES) TO
   sta   A4L,x             ; A4 AND A5
   sta   A5L,x
   dex
   bpl   LT2
   rts

MOVE:
:
   lda   (A1L),y           ;MOVE (A) THRU (A2) TO (A4)
   sta   (A4L),y
   jsr   NXTA4
   bcc   :- ;MOVE
   rts

VERIFY:
   lda   (A1L),y           ;VERIFY (A1) THRU (A2)
   cmp   (A4L),y           ; WITH (A4)
   beq   VFYOK
   jsr   PRA1
   lda   (A1L),y
   jsr   PRBYTE
   lda   #$A0
   jsr   cout
   lda   #$A8
   jsr   cout
   lda   (A4L),y
   jsr   PRBYTE
   lda   #$A9
   jsr   cout
VFYOK:
   jsr   NXTA4
   bcc   VERIFY
   rts


;******************************************************************************
;* Disassembler                                                               *
;******************************************************************************
LIST:
   jsr   a1pc              ;MOVE A1 (2 BYTES) TO
   lda   #$14              ; PC IF SPEC'D AND
LIST2:
   pha   ;+DISASSEMBLE 20 INSTRUCTIONS.
   jsr   showinst          ;+Display a line
   pla
   dec   ;+Count down
   bne   LIST2
   rts

showinst:
   jsr   instdsp
   jsr   pcadj
   sta   PCL
   sty   PCH
   rts


;******************************************************************************
a1pc:
   txa   ;IF USER SPECIFIED AN ADDRESS,
   beq   A1PCRTS           ; COPY IT FROM A1 TO PC.
A1PCLP:
   lda   A1L,x             ;YEP, SO COPY IT.
   sta   PCL,x
   dex
   bpl   A1PCLP
A1PCRTS:
   rts


;******************************************************************************
;* getup - get char from input buffer, iny and upshift it                     *
;******************************************************************************
GETUP:
   lda   IN,y
   iny
   ora   #$80              ;set high bit for execs
UPSHIFT:
   cmp   #$FB
   bcs   X_UPSHIFT
   cmp   #$E1
   bcc   X_UPSHIFT
   and   #$DF
X_UPSHIFT:
   rts


;******************************************************************************
REGDSP:
   jsr   CROUT             ;DISPLAY USER REG CONTENTS
   lda   #ACC              ;WITH LABELS
   sta   A3L               ;Memory state now printed
   lda   #$00
   sta   A3H
   ldx   #$fb              ; $100-5 bytes to be shown
RDSP1:
   lda   #$A0
   jsr   cout
   lda   rtbl-$fb,x        ; offset from above
   jsr   cout
   lda   #$bd              ; '='
   jsr   cout
.if ACC = $fb
   lda   $00,x
.else
   lda   ACC+5,x
.endif
   jsr   PRBYTE
   inx
   bne   RDSP1
   rts
rtbl:
   .byte "AXYPS"

;******************************************************************************
GETLNZ:
   jsr   CROUT
GETLN:
   lda   PROMPT
   jsr   cout
   stz   IN
   lda   #<IN
   ldx   #>IN
   ldy   SCRWIDTH
   int   LINEINPUT
   bcc   @eolmarker
   lda   #$DC              ;BACKSLASH AFTER CANCELLED LINE
   jsr   cout
   jsr   CROUT
   bra   GETLN
@eolmarker:
   jsr   CROUT
   sta   IN,y
@loop:
   lda   IN,y
   ora   #$80
   sta   IN,y
   dey
   bpl   @loop
   rts


;******************************************************************************
PRA1:
   ldy   A1H               ;PRINT CR,A1 IN HEX
   ldx   A1L
PRYX2:
   jsr   CROUT
   jsr   PRNTYX
   ldy   #$00
   lda   #$AD              ;PRINT "-"
   jmp   cout

GO:
   jsr   a1pc              ;ADDR TO PC IF SPECIFIED

   pla                     ; at this point, only the JSR GO is on the stack
   ply
   ldx   SPNT              ; set stack to defined position
   txs
   phy                     ; write return address to new stack position
   pha

   lda   STATUS            ;RESTORE 6502 REGISTER CONTENTS
   pha                     ; USED BY DEBUG SOFTWARE
   lda   A5H
   ldx   XREG
   ldy   YREG
   plp

   jsr   @jsrindirect      ; AND GO!

   php
   sta   A5H               ;SAVE 6502 REGISTER CONTENTS
   stx   XREG              ; FOR DEBUG SOFTWARE
   sty   YREG
   pla
   sta   STATUS
   tsx
   inx                     ; adjust for return address after rts
   inx
   stx   SPNT

   cld
   rts
@jsrindirect:
   jmp   (PCL)
;
BL1:
   dec   YSAV
   beq   XAM8
;
BLANK:
   dex                     ;BLANK TO MON
   bne   SETMDZ            ;AFTER BLANK
   cmp   #$BA              ;DATA STORE MODE?
   bne   XAMPM             ; NO; XAM, ADD, OR SUBTRACT.
   sta   MODE              ;KEEP IN STORE MODE
   lda   A2L
   sta   (A3L),y           ;STORE AS LOW BYTE AT (A3)
   inc   A3L
   bne   RTS5              ;INCR A3, RETURN.
   inc   A3H
RTS5:
   rts

CRMON:
   jsr   BL1               ;HANDLE CR AS BLANK
   pla                     ; THEN POP STACK
   pla                     ;AND RETURN TO MON
   bra   MONZ              ;(ALWAYS)
;

SETMODE:
   ldy   YSAV              ;SAVE CONVERTED ':', '+',
   lda   IN-1,y            ; '-', '.' AS MODE
SETMDZ:
   sta   MODE
   rts


;******************************************************************************
XAM8:
   lda   A1L
   ora   #$0f              ;SET TO FINISH AT
   sta   A2L               ; MOD 8=7
   lda   A1H
   sta   A2H
MOD8CHK:
   lda   A1L
   and   #$0f
   bne   DATAOUT
XAM:
   jsr   PRA1
DATAOUT:
   tax
:
   dex
   lda   #$A0
   jsr   cout              ;OUTPUT BLANK
   cpx   #$07
   beq   :-
   lda   (A1L)             ;was: lda   (A1L),y
   jsr   PRBYTE            ;OUTPUT BYTE IN HEX
   jsr   NXTA1
   bcc   MOD8CHK           ;NOT DONE YET. GO CHECK MOD 8
   rts                     ;DONE.


;******************************************************************************
XAMPM:
   lsr                     ;DETERMINE IF MONITOR MOD IS
   bcc   XAM               ; EXAMINE, ADD OR SUBTRACT
   lsr
   lsr
   lda   A2L
   bcc   ADD
   eor   #$FF              ;FORM 2'S COMPLEMENT FOR SUBTRACT.
ADD:
   adc   A1L
   pha
   lda   #$BD              ;PRINT '=', THEN RESULT
   jsr   cout
   pla
;
PRBYTE:
   pha                     ;PRINT BYTE AS 2 HEX DIGITA
   lsr                     ; (DESTROYS A-REG)
   lsr
   lsr
   lsr
   jsr   PRHEXZ
   pla
;
PRHEX:
   and   #$0F              ;PRINT HEX DIGIT IN A-REG
PRHEXZ:
   ora   #$B0              ;LSBITS ONLY.
   cmp   #$BA
   bcc   cout
   adc   #$06
;
cout:
   pha
   and   #$7f              ; adjust output for Sorbus Computer
   cmp   #$0d              ; convert cr -> nl
   bne   :+
   lda   #$0a
:
   jsr   CHROUT
   pla                     ; routines expect A unchanged
   rts

CROUT:
   lda   #$8D
   bra   cout              ;(ALWAYS) was bne


;******************************************************************************
;* main monitor code                                                          *
;******************************************************************************

MON:
   cld                     ;MUST SET HEX MODE!
MONZ:
   lda   #$AA              ;'*' PROMPT FOR MONITOR
   sta   PROMPT
   jsr   GETLNZ            ;READ A LINE OF INPUT
   jsr   ZMODE             ;CLEAR MONITOR MODE, SCAN IDX (set Y=$00)
NXTITM:
   jsr   GETNUM            ;GET ITEM, NON-HEX
   sty   YSAV              ; CHAR IN A-REG
   ldy   #(SUBTBL-CHRTBL)  ;X-REG=0 IF NO HEX INPUT
CHRSRCH:
   dey
.if CONFIG_CMD_NOT_FOUND
   bpl   :+
   jsr   PRINT
   .byte "cmd not found:",0
   int   PRHEX8
   bra   MON
:
.else
   bmi   MON               ;COMMAND NOT FOUND, BEEP & TRY AGAIN.
.endif
   cmp   CHRTBL,y          ;FIND COMMAND CHAR IN TABLE
   bne   CHRSRCH           ;NOT THIS TIME
   jsr   TOSUB             ;GOT IT! CALL CORRESPONDING SUBROUTINE
   ldy   YSAV              ;PROCESS NEXT ENTRY ON HIS LINE
   jmp   NXTITM

DIG:
   ldx   #$03
   asl
   asl                     ;GOT HEX DIGIT,
   asl                     ; SHIFT INTO A2
   asl
NXTBIT:
   asl
   rol   A2L
   rol   A2H
   dex   ;LEAVE X=$FF IF DIG
   bpl   NXTBIT
NXTBAS:
   lda   MODE
   bne   NXTBS2            ;IF MODE IS ZERO,
   lda   A2H,x             ; THEN COPY A2 TO A1 AND A3
   sta   A1H,x
   sta   A3H,x
NXTBS2:
   inx
   beq   NXTBAS
   bne   NXTCHR

GETNUM:
   ldx   #$00              ;CLEAR A2
   stx   A2L
   stx   A2H
NXTCHR:
   jsr   GETUP             ;Get char, iny, upshift
   eor   #$B0
   cmp   #$0A
   bcc   DIG               ;it's a digit
   adc   #$88
   cmp   #$FA
;   jmp   LOOKASC           ;+ Check for quote
;******************************************************************************
;* LOOKASC - addition to monitor input routine, if a quote (')                  *
;* in input, the ascii of the next is input like a hex number                   *
;******************************************************************************
LOOKASC:
   bcs   DIG               ;Was char a hex digit?
   cmp   #$A0              ;Is it a quote
   bne   ladone            ;Done if not
   lda   IN,y              ;Get next char
   ldx   #7                ;for shifting asci into A2L and A2H
   cmp   #$8D              ;Was it a cr?
   beq   GETNUM            ;Go handle cr
   iny                     ;Advance index into IN
   and   #$7f              ; convert to ASCII for Sorbus Computer
   bra   NXTBIT            ;Go shift it in
ladone:
   rts

TOSUB:
   tya
   asl
   tay
   lda   SUBTBL+1,y
   pha
   lda   SUBTBL+0,y
   pha
   lda   MODE
ZMODE:
   ldy   #$00              ; MODE IN A-REG),
   sty   MODE
   rts   ; AND 'RTS' TO THE SUBROUTINE!


;******************************************************************************
MNEM:
   ; format is:
   ; convert each letter to 5 bits by
   ; - ascii & 0x1f + 1
   ; word offsets:
   ; - 1st letter << 11
   ; - 2nd letter <<  6
   ; - 3rd letter <<  1
   .word $1CD8             ; $00:"BRK"
   .word $8A62             ; $01:"PHP"
   .word $1C5A             ; $02:"BPL"
   .word $2348             ; $03:"CLC"
   .word $5D26             ; $04:"JSR"
   .word $8B62             ; $05:"PLP"
   .word $1B94             ; $06:"BMI"
   .word $A188             ; $07:"SEC"
   .word $9D54             ; $08:"RTI"
   .word $8A44             ; $09:"PHA"
   .word $1DC8             ; $0A:"BVC"
   .word $2354             ; $0B:"CLI"
   .word $9D68             ; $0C:"RTS"
   .word $8B44             ; $0D:"PLA"
   .word $1DE8             ; $0E:"BVS"
   .word $A194             ; $0F:"SEI"
   .word $1CC4             ; $10:"BRA"
   .word $29B4             ; $11:"DEY"
   .word $1908             ; $12:"BCC"
   .word $AE84             ; $13:"TYA"
   .word $6974             ; $14:"LDY"
   .word $A8B4             ; $15:"TAY"
   .word $1928             ; $16:"BCS"
   .word $236E             ; $17:"CLV"
   .word $2474             ; $18:"CPY"
   .word $53F4             ; $19:"INY"
   .word $1BCC             ; $1A:"BNE"
   .word $234A             ; $1B:"CLD"
   .word $2472             ; $1C:"CPX"
   .word $53F2             ; $1D:"INX"
   .word $19A4             ; $1E:"BEQ"
   .word $A18A             ; $1F:"SED"
   .word $AD06             ; $20:"TSB"
   .word $1AAA             ; $21:"BIT"
   .word $5BA2             ; $22:"JMP"
   .word $5BA2             ; $23:"JMP"
   .word $A574             ; $24:"STY"
   .word $6974             ; $25:"LDY"
   .word $2474             ; $26:"CPY"
   .word $2472             ; $27:"CPX"
   .word $AE44             ; $28:"TXA"
   .word $AE68             ; $29:"TXS"
   .word $A8B2             ; $2A:"TAX"
   .word $AD32             ; $2B:"TSX"
   .word $29B2             ; $2C:"DEX"
   .word $8A72             ; $2D:"PHX"
   .word $7C22             ; $2E:"NOP"
   .word $8B72             ; $2F:"PLX"
   .word $151A             ; $30:"ASL"
   .word $9C1A             ; $31:"ROL"
   .word $6D26             ; $32:"LSR"
   .word $9C26             ; $33:"ROR"
   .word $A572             ; $34:"STX"
   .word $6972             ; $35:"LDX"
   .word $2988             ; $36:"DEC"
   .word $53C8             ; $37:"INC"
   .word $84C4             ; $38:"ORA"
   .word $13CA             ; $39:"AND"
   .word $3426             ; $3A:"EOR"
   .word $1148             ; $3B:"ADC"
   .word $A544             ; $3C:"STA"
   .word $6944             ; $3D:"LDA"
   .word $23A2             ; $3E:"CMP"
   .word $A0C8             ; $3F:"SBC"
   .word $A576             ; $40:"STZ"
   .word $ACC6             ; $41:"TRB"
   .word $8A74             ; $42:"PHY"
   .word $8B74             ; $43:"PLY"

;******************************************************************************
; FMT1 BYTES:    XXXXXXY0 INSTRS
; IF Y=0         THEN RIGHT HALF BYTE
; IF Y=1         THEN LEFT HALF BYTE
;                   (X=INDEX)
;
FMT1:
   .byte $0F
   .byte $22
   .byte $FF
   .byte $33
   .byte $CB
   .byte $62
   .byte $FF
   .byte $73
   .byte $03
   .byte $22
   .byte $FF
   .byte $33
   .byte $CB
   .byte $66
   .byte $FF
   .byte $77
   .byte $0F
   .byte $20
   .byte $FF
   .byte $33
   .byte $CB
   .byte $60
   .byte $FF
   .byte $70
   .byte $0F
   .byte $22
   .byte $FF
   .byte $39
   .byte $CB
   .byte $66
   .byte $FF
   .byte $7D
   .byte $0B
   .byte $22
   .byte $FF
   .byte $33
   .byte $CB
   .byte $A6
   .byte $FF
   .byte $73
   .byte $11
   .byte $22
   .byte $FF
   .byte $33
   .byte $CB
   .byte $A6
   .byte $FF
   .byte $87
   .byte $01
   .byte $22
   .byte $FF
   .byte $33
   .byte $CB
   .byte $60
   .byte $FF
   .byte $70
   .byte $01
   .byte $22
   .byte $FF
   .byte $33
   .byte $CB
   .byte $60
   .byte $FF
   .byte $70
   .byte $24
   .byte $31
   .byte $65
   .byte $78
; ZZXXXY01 INSTR'S
FMT2:
   .byte $00               ;ERR
   .byte $21               ;IMM
   .byte $81               ;Z-PAGE
   .byte $82               ;ABS
   .byte $59               ;(ZPAG,X)
   .byte $4D               ;(ZPAG),Y
   .byte $91               ;ZPAG,X
   .byte $92               ;ABS,X
   .byte $86               ;ABS,Y
   .byte $4A               ;(ABS)
   .byte $85               ;ZPAG,Y
   .byte $9D               ;RELATIVE
   .byte $49               ;(ZPAG)      (new)
   .byte $5A               ;(ABS,X)     (new)
;
CHAR2:
   .byte $D9               ;'Y'
   .byte $00               ; (byte F of FMT2)
   .byte $D8               ;'Y'
   .byte $A4               ;'$'
   .byte $A4               ;'$'
   .byte $00
;
CHAR1:
   .byte $AC               ;','
   .byte $A9               ;')'
   .byte $AC               ;','
   .byte $A3               ;'#'
   .byte $A8               ;'('
   .byte $A4               ;'$'

;
; OPTBL is a table containing the new opcodes that
; wouldn't fit into the existing lookup table.
;
OPTBL:
   .byte $12               ;ORA (ZPAG)
   .byte $14               ;TRB ZPAG
   .byte $1A               ;INC A
   .byte $1C               ;TRB ABS
   .byte $32               ;AND (ZPAG)
   .byte $34               ;BIT ZPAG,X
   .byte $3A               ;DEC A
   .byte $3C               ;BIT ABS,X
   .byte $52               ;EOR (ZPAG)
   .byte $5A               ;PHY
   .byte $64               ;STZ ZPAG
   .byte $72               ;ADC (ZPAG)
   .byte $74               ;STZ ZPAG,X
   .byte $7A               ;PLY
   .byte $7C               ;JMP (ABS,X)
   .byte $89               ;BIT IMM
   .byte $92               ;STA (ZPAG)
   .byte $9C               ;STZ ABS
   .byte $9E               ;STZ ABS,X
   .byte $B2               ;LDA (ZPAG)
   .byte $D2               ;CMP (ZPAG)
   .byte $F2               ;SBC (ZPAG)
   .byte $FC               ;??? (the unknown opcode)
;
; INDX contains pointers to the mnemonics for each of
; the opcodes in OPTBL.  Pointers with BIT 7
; set indicate extensions to MNEML or MNEMR.
;
INDX:
   .byte $38               ;ORA (ZPAG)
   .byte $41               ;TRB ZPAG
   .byte $37               ;INC A
   .byte $41               ;TRB ABS
   .byte $39               ;AND (ZPAG)
   .byte $21               ;BIT ZPAG,X
   .byte $36               ;DEC A
   .byte $21               ;BIT ABS,X
   .byte $3A               ;EOR (ZPAG)
   .byte $42               ;PHY
   .byte $40               ;STZ ZPAG
   .byte $3B               ;ADC (ZPAG)
   .byte $40               ;STZ ZPAG,X
   .byte $43               ;PLY
   .byte $22               ;JMP (ABS,X)
   .byte $21               ;BIT IMM
   .byte $3C               ;STA (ZPAG)
   .byte $40               ;STZ ABS
   .byte $40               ;STZ ABS,X
   .byte $3D               ;LDA (ZPAG)
   .byte $3E               ;CMP (ZPAG) ;??? (the unknown opcode)
   .byte $3F               ;SBC (ZPAG)
   .byte $FC               ;???
   .byte $00


CHRTBL:
   .byte $F1               ;X (exit)
   .byte $9D               ;$   (DIR)
   .byte $D4               ;{   (LOAD)
   .byte $D6               ;}   (SAVE)
   .byte $EB               ;R was: $BE ;^E  (OPEN AND DISPLAY REGISTERS)
   .byte $9A               ;+!  (Mini assembler)
   .byte $EF               ;V   (MEMORY VERIFY)
   .byte $A6               ;'-' (SUBSTRACTION)
   .byte $A4               ;'+' (ADDITION)
   .byte $06               ;M   (MEMORY MOVE)
   .byte $95               ;'<' (DELIMITER FOR MOVE, VFY)
   .byte $05               ;L   (DISASSEMBLE 20 INSTRS)
   .byte $00               ;G   (EXECUTE PROGRAM)
   .byte $93               ;':' (MEMORY FILL)
   .byte $A7               ;'.' (ADDRESS DELIMITER)
   .byte $C6               ;'CR' (END OF INPUT)
   .byte $99               ;BLANK

SUBTBL:
   .word EXIT-1            ;X (exit)
   .word DIR-1
   .word LOAD-1
   .word SAVE-1
   .word REGDSP-1          ;R was: $BE ;^E  (OPEN AND DISPLAY REGISTERS)
   .word GETINST1-1        ;+!  (Mini assembler)
   .word VERIFY-1          ;V   (MEMORY VERIFY)
   .word SETMODE-1         ;'-' (SUBSTRACTION)
   .word SETMODE-1         ;'+' (ADDITION)
   .word MOVE-1            ;M   (MEMORY MOVE)
   .word LT-1              ;'<' (DELIMITER FOR MOVE, VFY)
   .word LIST-1            ;L   (DISASSEMBLE 20 INSTRS)
   .word GO-1              ;G   (EXECUTE PROGRAM)
   .word SETMODE-1         ;':' (MEMORY FILL)
   .word SETMODE-1         ;'.' (ADDRESS DELIMITER)
   .word CRMON-1           ;'CR' (END OF INPUT)
   .word BLANK-1           ;BLANK
