
; set processor to NMOS 6502
.p02

.include "jam.inc"
.include "jam_bios.inc"

;-------------------------------------------------------------------------
; BIOS calls $FF00-$FFFF
; this BIOS is only for the NMOS 6502 toolkit
; NMOS CPU does not work with CMOS BIOS due to moved code
;-------------------------------------------------------------------------

.segment "BIOS"
BIOS:
_chrin:
   ; read character from UART
   jmp   chrin
_chrout:
   ; write character to UART
   jmp   chrout
_print:
   ; print a string while keeping all registers
   sta   ASAVE          ; save A, it is used together with
   php                  ;   processor status register,
   pla                  ;   which can be only read via stack
   sta   PSAVE          ; save it
   pla                  ; get lobyte of address of text from stack and
   sta   TMP16+0        ;   store it into temporary vector
   pla                  ; same for hibyte
   sta   TMP16+1        ;   store
   tya                  ; make function 6502 proof, store Y the NMOS way
   pha
   ldy   #$00           ; required for "lda (TMP16),y" below
@loop:
   inc   TMP16+0        ; since JSR stores return address - 1, start with
   bne   :+             ;   incrementing the pointer
   inc   TMP16+1
:
   lda   (TMP16),y      ; this could be "lda (tmp16)", but does not work with
                        ; NMOS 6502 or 65CE02
   beq   @out           ; $00 bytes indicates end of text
   jsr   chrout         ; output the character
   bpl   @loop          ; always true
@out:
   pla
   tay                  ; restore save Y register
   lda   TMP16+1        ; get updated vector now pointing to end of text
   pha                  ; write hibyte to stack
   lda   TMP16+0        ; (reverse order of fetching)
   pha                  ; write lobyte to stack
   lda   PSAVE          ; get saved processor status
   pha                  ; put it on stack for restoring
   lda   ASAVE          ; get saved A
   plp                  ; get processor status from stack into register
   rts                  ; now return to code after text

chrout:
   bit   UARTWS         ; wait for buffer to accept data
   bmi   chrout
   sta   UARTWR         ; write data to output queue
   rts

chrin:
   lda   UARTRS         ; check for size of input queue
   bne   :+             ; data available, fetch it
   sec                  ; no input -> return 0 and set carry
   rts
:
   lda   UARTRD         ; get key value
   clc
   rts

IRQCHECK:
   sta   BRK_SA         ; let's figure out the source of the IRQ
   pla                  ; get processor status from stack
   pha                  ; and put it back
   and   #$10           ; check for BRK bit
   bne   @isbrk
   lda   BRK_SA         ; restore saved accumulator
   jmp   (UVNBI)        ; user vector for non-BRK IRQ ($DF7C)
@isbrk:
   jmp   (UVBRK)        ; user vector for BRK IRQ ($DF7E)

bankrti:
   ; this version is stripped down -> no bank restored, would break NMOS 6502
   php
   ldy   BRK_SY         ; get stored Y
   pla
   sta   BRK_SY         ; use stored Y now for storing P
   ldx   BRK_SX         ; get stored X
   pla                  ; drop old P from stack
   lda   BRK_SY         ; get P from store
   pha                  ; put it on the stack, so flags can be returned
   lda   BRK_SA         ; get stored accumulator
   rti                  ; return to calling code
_biosend:

.segment "FIXEND"
_fixstart:
NMI:
   jmp   (UVNMI)        ; user vector for NMI ($DF7A)
IRQ:
   jmp   (UVIRQ)        ; user vector for BRK/IRQ ($DF7E) -> irqcheck (default)
RESET:
   ; reset routine that also has to work when copied to RAM
   lda   #$01
   sta   BANK           ; set banking to first ROM bank (kernel)
   jmp   reset          ; jump to reset routine
_fixend:

.segment "VECTORS"
_vectors:
   .word NMI
   .word RESET
   .word IRQ

_biossize = (_biosend-BIOS) + (_fixend-_fixstart)
.out "   ============================="
.out .sprintf( "   BIOS size: $%04x ($%04x free)", _biossize, $FA - (_biossize) )
.out "   ============================="

.assert  CHRIN   = _chrin,   error, "CHRIN at wrong address"
.assert  CHROUT  = _chrout,  error, "CHROUT at wrong address"
.assert  PRINT   = _print,   error, "PRINT at wrong address"
.assert  _fixend = _vectors, error, "FIXEND segment not at end of memory"
