
.export     disassemble

.import     CHROUT
.import     PRINT
.import     getaddr
.import     prtsp
.import     prhex8s
.import     prhex8
.import     newopcode

.importzp   ADDR0
.importzp   ADDR1
.importzp   ADDR2
.importzp   TMP8
.importzp   TMP16

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
;* however those routines were heavily modified to fit the Sorbus Computer    *
;******************************************************************************

LMNEM    := TMP16+1
RMNEM    := TMP16+0

FORMAT   := $e8         ;{addr/1}   ;temp for opcode decode
LENGTH   := $e9         ;{addr/1}   ;temp for opcode decode

prt3sp:
   ldx   #$03
prtxsp:
   jsr   prtsp
   dex
   bne   prtxsp
   rts

disassemble:
   jsr   getaddr
   ldy   #$14
   bcs   @next
   lda   ADDR0+0
   sta   ADDR1+0
   lda   ADDR0+1
   sta   ADDR1+1

@next:
.ifp02
   tya
   pha
.else
   phy
.endif
   jsr   PRINT
   .byte 10," > ",0
   jsr   prtinst
   lda   ADDR1+0
   sec
   adc   LENGTH
   sta   ADDR1+0
   bcc   :+
   inc   ADDR1+1
:
.ifp02
   pla
   tay
.else
   ply
.endif
   dey
   bne   @next

   rts

prtinst:
   ; print address
   lda   ADDR1+1
   jsr   prhex8
   lda   ADDR1+0
   jsr   prhex8

   jsr   prt3sp         ;followed by 3 spaces (will set X=$00)

   ; X=0 expected
   lda   (ADDR1,x)        ;GET OPCODE
   tay                  ;LABLE moved down 1
   lsr                  ;EVEN/ODD TEST
   bcc   @ieven
   ror                  ;BIT 1 TEST
   bcs   @error         ;XXXXXX11 INVALID OP
   and   #$87           ;MASK BITS
@ieven:
   lsr                  ;LASB INTO CARRY FOR L/R TEST
   tax
   lda   FMT1,x
   ; taken from SCRN2
   bcc   :+             ;IF EVEN, USE LO H
   lsr
   lsr
   lsr                  ;SHIFT HIGH HALF BYTE DOWN
   lsr
:
   and   #$0F           ;MASK 4-BITS
   bne   :+
@error:
   ldy   #$FC           ;SBSTITUTE $FC FOR INVALID OPS
   lda   #$00           ;SET PRINT FORMAT INDEX TO 0
:
   tax
   lda   FMT2,x         ;INDEX INTO PRINT FORMAT TABLE
   sta   FORMAT         ;SAVE FOR ADR FIELD FORMATTING
   and   #$03           ;MASK FOR 2-BIT LENGTH
; (0=1 BYTE, 1=2 BYTE, 2=3 BYTE)
   sta   LENGTH
.ifp02
   ; no need for a 65c02 custom subroutine
.else
   jsr   newopcode      ;get index for new opcodes
   beq   @foundnew      ;found a new op (or no op)
.endif
   and   #$8F           ;MASK FOR 1XXX1010 TEST
   tax                  ; SAVE IT
   tya                  ;OPCODE TO A AGAIN
   ldy   #$03
   cpx   #$8A
   beq   @mntcv3
@mntcv1:
   lsr
   bcc   @mntcv3        ;FORM INDEX INTO MNEMONIC TABLE
   lsr
@mntcv2:
   lsr                  ;  1) 1XXX1010 => 00101XXX
   ora   #$20           ;  2) XXXYYY01 => 00111XXX
   dey                  ;  3) XXXYYY10 => 00110XXX
   bne   @mntcv2        ;  4) XXXYY100 => 00100XXX
   iny                  ;  5) XXXXX000 => 000XXXXX
@mntcv3:
   dey
   bne   @mntcv1
@foundnew:

   pha                  ;SAVE MNEMONIC TABLE INDEX
@top:
   lda   (ADDR1),y
   jsr   prhex8s
   ldx   #$01           ;print a single space
@skipoutput:
   jsr   prtxsp         ; also sets X=0
   cpy   LENGTH         ;PRINT INST (1-3 BYTES)
   iny                  ;IN A 12 CHR FIELD
   bcc   @top
   ldx   #$03           ;CHAR COUNT FOR MNEMONIC INDEX
   cpy   #$04
   bcc   @skipoutput

   pla                  ;RECOVER MNEMONIC INDEX
   asl
   tay
   lda   MNEM+1,y
   sta   LMNEM          ;FETCH 3-CHAR MNEMONIC
   lda   MNEM+0,y       ;  (PACKED INTO 2-BYTES)
   sta   RMNEM
@exptext1:
   lda   #$00
   ldy   #$05
@exptext2:
   asl   RMNEM          ;SHIFT 5 BITS OF CHARACTER INTO A
   rol   LMNEM
   rol                  ;  (CLEARS CARRY)
   dey
   bne   @exptext2
   adc   #$3F           ;ADD "?" OFFSET
   jsr   CHROUT         ;OUTPUT A CHAR OF MNEM
   dex
   bne   @exptext1
   jsr   prt3sp         ; print 3 spaces, sets X=0
   ldy   LENGTH
   ldx   #$06           ;CNT FOR 6 FORMAT BITS
@pradr1:
   cpx   #$03
   beq   @pradr5        ;IF X=3 THEN ADDR.
@pradr2:
   asl   FORMAT
   bcc   @pradr3
   lda   CHAR1-1,x
   jsr   CHROUT
   lda   CHAR2-1,x
   beq   @pradr3
   jsr   CHROUT
@pradr3:
   dex
   bne   @pradr1
   rts

@pradr4:
   dey
   bmi   @pradr2
   jsr   prhex8s
@pradr5:
   lda   FORMAT
   cmp   #$E8           ;HANDLE REL ADR MODE
   lda   (ADDR1),y      ;SPECIAL (PRINT TARGET,
   bcc   @pradr4        ;  NOT OFFSET)
   jsr   @pcadj3
   tax                  ;ADDR1+OFFSET+1 TO A,Y
   inx
   bne   :+             ;+1 TO Y,X
   iny
:
   tya
   jsr   prhex8
   txa                  ;  OF BRANCH AND RETURN
   jmp   prhex8

@pcadj3:
   ldy   ADDR1+1
   tax                  ;TEST DISPLACEMENT SIGN
   bpl   @pcadj4        ;  (FOR REL BRANCH)
   dey                  ;EXTEND NEG BY DECR ADDR1+1
@pcadj4:
   adc   ADDR1+0
   bcc   :+             ;ADDR1+0+LENGTH(OR DISPL)+1 TO A
   iny                  ;  CARRY INTO Y (ADDR1+1)
:
   rts
