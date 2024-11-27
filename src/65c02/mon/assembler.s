
.include "../native_bios.inc"
.include "../native.inc"

.export     assemble

.import     newedit

.import     prtxsp
.import     decodeopcode
.import     prterr
.import     skipspace
.import     prtinst
.import     getaddr
.import     inbufa
.import     inbufaddr1
.import     uppercase
.import     newenter

.importzp   ADDR0       ; acts as address parameter
.importzp   ADDR1       ; acts as PC
.importzp   ADDR2       ; acts as storage for 16bit MNEM
.importzp   MODE
.importzp   FORMAT
.importzp   LENGTH
.importzp   TMP8

.import     INBUF

.import     MNEM
.import     FMT1
.import     FMT2
.import     CHAR1
.import     CHAR2

;******************************************************************************
;* the routines used in this file are based upon:                             *
;******************************************************************************
;*                                                                            *
;*  Apple //c                                                                 *
;*  Monitor ROM Source                                                        *
;*                                                                            *
;*  Copyright 1977-1983 by Apple Computer, Inc.                               *
;*  All Rights Reserved                                                       *
;*                                                                            *
;*  S. Wozniak           1977                                                 *
;*  A. Baum              1977                                                 *
;*  John A           NOV 1978                                                 *
;*  R. Auricchio     SEP 1982                                                 *
;*  E. Beernink          1983                                                 *
;*                                                                            *
;******************************************************************************
;* however those routines were heavily modified to fit the Sorbus Computer    *
;******************************************************************************

A1H      = MODE
A5L      = BRK_SA
XSAV     = BRK_SX
YSAV     = BRK_SY

; A1H, ADDR0+0, ADDR0+1 form the buffer for the instruction
; --> ADDR0+0/H = ADDR0 (used from 2nd call to getaddr)
; --> A1H   = ADDR0-1 (must be byte before, MODE)
.assert A1H = ADDR0-1, error, "A1H must be followed by ADDR0 in memory"

; this wrapper is getting on my nerves, I have to check and doublecheck
; just to make sure it provides what the rest requires
getaddr1:
   lda   INBUF,x
   beq   @fail
   jsr   getaddr
   ; do some very different error checking here
   lda   TMP8
   cmp   #$04           ; check if any digit was processed
   bcs   @fail
@ok:
   inx                  ; compensate GETNUM
   ldy   #$01
   rts
@fail:
   lda   #$00
   sta   ADDR0+0
   sta   ADDR0+1
   tay
   rts

skipspace1:
   jsr   skipspace
   inx
   jmp   uppercase

assemble:
   jsr   getaddr        ; get addr to assemble to
   bcc   :+
   rts                  ; on error just leave for now
:
   sta   ADDR1+0        ; getaddr will be used again, save to other
   sty   ADDR1+1        ; addr pointer

   lda   #$03           ;get starting opcode
   sta   A1H            ;and save
@nxtch:
   jsr   skipspace1     ;get next non-blank
   asl
   ;clc                 ; implicit, because skipspace returns ascii
   sbc   #$BD
   cmp   #$C2
.if 1
   bcc   @goprterr
.else
   bcs   :+
   jmp   @goprterr
:
.endif
;
; Form mnemonic for later comparison
;
   asl
   asl
   ldy   #$04
@nxtmn:
   asl
   rol   ADDR2+0
   rol   ADDR2+1
   dey
   bpl   @nxtmn
   dec   A1H            ;decrement mnemonic count
   beq   @nxtmn
   bpl   @nxtch
   ldy   #$05           ;index into address mode tables
   jsr   @amod1         ;do this elsewhere
.if 0
   jsr   PRINT
   .byte 10,"ID:",0
   lda   ADDR2+0
   int   PRHEX8
   lda   ADDR2+1
   int   PRHEX8
   lda   A5L
   int   PRHEX8
   lda   YSAV
   int   PRHEX8
.endif
   lda   A5L            ;get format
   asl
   asl
   ora   YSAV
   cmp   #$20
   bcs   @amod7
   ldy   YSAV           ;get our format
   beq   @amod7
   ora   #$80
@amod7:
   sta   A5L            ;update format
   stx   XSAV           ;update position
   inx
   lda   INBUF,x        ;get next character
   beq   @amod8         ;is it end of input?
   cmp   #$3B           ;is it a ";"? (rest of line is comment)
   bne   @goprterr
@amod8:
@getop:
   lda   A1H            ;get opcode
   jsr   decodeopcode   ;determine mnemonic index
   tay
   lda   MNEM+0,y
   cmp   ADDR2+0
   bne   @nxtop
   lda   MNEM+1,y
   cmp   ADDR2+1        ;does it match entry?
   bne   @nxtop         ;=>no, try next opcode

   lda   A5L            ;found opcode, check address mode
   ldy   FORMAT         ;get addr. mode format for that opcode
   cpy   #$9D           ;is it relative?
.if 1
   beq   rel            ;=>yes, calc relative address
.else
   bne   :+
   jmp   rel
:
.endif
   cmp   FORMAT         ;does mode match?
.if 1
   beq   movinst        ;=>yes, move instruction to memory
.else
   bne   :+
   jmp   movinst
:
.endif
@nxtop:
   dec   A1H            ;else try next opcode
   bne   @getop         ;=>go try it
   inc   A5L            ;else try next format
   dec   XSAV
   beq   @getop         ;=>go try next foramt
@goprterr:
   ldx   XSAV
   jmp   prterr

@amod1:
   jsr   skipspace1     ;get next non-blank
   stx   XSAV           ;save textpos
   cmp   CHAR1,y
   bne   @amod2
   jsr   skipspace1     ;get next non-blank
   cmp   CHAR2,y
   beq   @amod3
   lda   CHAR2,y        ;done yet?
   beq   @amod4
   cmp   #$24           ;if "$" then done
   beq   @amod4
   ldx   XSAV           ;restore textpos
@amod2:
   clc
@amod4:
   dex
@amod3:
   rol   A5L            ;shift bit into format
   cpy   #$03
   bne   @amod6
   jsr   getaddr1       ; get 16-bit value into addr0
   lda   ADDR0+1        ;get high byte of address
   beq   @amod5         ;=>
   iny
@amod5:
   sty   YSAV
   ldy   #$03
   dex
@amod6:
   sty   A1H
   dey
   bpl   @amod1
   rts

;
; calculate offset byte for relative addresses
;
rel:
   sbc   #$81           ;calc relative address
   lsr
   bne   @goerr         ;bad branch
   ldy   ADDR0+1
   ldx   ADDR0+0
   bne   :+
   dey                  ;point to offset
:
   dex                  ;displacement -1
   txa
   clc
   sbc   ADDR1+0        ;subtract current ADDR1+0
   sta   ADDR0+0        ;and save as displacement
   bpl   :+             ;check page
   iny
:
   tya                  ;get page
   sbc   ADDR1+1        ;check page
   beq   :+
@goerr:
   ldx   XSAV
   jmp   prterr
:
;
; Move instruction to memory
;
movinst:
   ldy   LENGTH         ;get instruction length (0-2)
:
   lda   A1H,y          ;get a byte (write MODE, ADDR0+0, ADDR0+1 to address in ADDR1)
   sta   (ADDR1),y
   dey
   bpl   :-
;
; Display information
;
   lda   #'>'           ; restore mode which was misused for A1H
   sta   MODE
   jsr   PRINT
   .byte $1b,"[A",0
   jsr   prtinst        ;Display line --> done
; calculate next address
   sec
   lda   LENGTH
   adc   ADDR1+0
   sta   ADDR1+0
   bcc   :+
   inc   ADDR1+1
:
   ldx   #$00
   lda   #'A'
   jsr   inbufa
   lda   #' '
   jsr   inbufa
   jsr   inbufaddr1
   lda   #' '
:
   jsr   inbufa
   cpx   #$15
   bcc   :-
.ifp02
   lda   #$00
   jsr   inbufa
.else
   stz   INBUF,x
.endif
   pla                  ; remove return address from stack
   pla                  ; to prevent clearing of input buffer
   jmp   newenter       ; and jump to editing prepared input
