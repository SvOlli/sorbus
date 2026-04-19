
.include "jam.inc"
.include "jam_bios.inc"

;-------------------------------------------------------------------------
; BIOS calls $FF00-$FFFF
;-------------------------------------------------------------------------
; CHRIN, CHROUT and PRINT should work on all 6502 variants
; notable exceptions are:
; NMOS 6502:
; - IRQ handler code does not work, because code in kernel is for CMOS
; 65CE02:
; - Z register must be $00 when BRK is triggered
;   (there is a wrapper in jam_bios.inc)
; 65816:
; - using extra vectors is only possible when running from bank $00 (RAM)

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
                        ; NMOS 6502 or 65CE02 with Z != $00
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

ramio:                  ; internal-only routine to access RAM under ROM
   ldx   BANK           ; save current ROM bank
   stz   BANK           ; switch to RAM
ramio_cmd:
   lda   ($00),y        ; intended to be switched to sta ($00),y
   stx   BANK           ; restore current ROM bank
   rts                  ; return
; optional idea: push X to stack and expand ramio_cmd to 3 bytes for JSR


IRQCHECK:
   sta   BRK_SA         ; let's figure out the source of the IRQ
   pla                  ; get processor status from stack
   pha                  ; and put it back
   and   #$10           ; check for BRK bit
   bne   _isbrk
   lda   BRK_SA         ; restore saved accumulator
   jmp   (UVNBI)        ; user vector for non-BRK IRQ ($DF7C)
_isbrk:
   lda   BANK           ; get current bank
   sta   BRK_SB         ; save it
   stx   BRK_SX
   sty   BRK_SY
   tsx
   lda   $0103,x        ; offset of current stack pointer is 1+2
   sta   TMP16+1        ; at offset 0 is processor status (got called via BRK)
   lda   $0102,x        ; get the return address from stack
   sta   TMP16+0        ; and write it to temp vector

   bne   :+
   dec   TMP16+1
:
   dec   TMP16+0
   lda   (TMP16)        ; finally get BRK operand!
                        ; needs to be read without bank change
   sta   ASAVE          ; misuse asave to store BRK operand for user handler
   ldx   #$01           ; will be restored by brkjump
   stx   BANK           ; switch to first ROM bank
   jsr   brkjump        ; to call the subroutine selector

bankrti:
   php
   ldy   BRK_SY         ; get stored Y
   pla
   sta   BRK_SY         ; use stored Y now for storing P
   ldx   BRK_SX         ; get stored X
   lda   BRK_SB         ; get stored BANK
   sta   BANK           ; restore saved bank
   pla                  ; drop old P from stack
   lda   BRK_SY         ; get P from store
   pha                  ; put it on the stack, so flags can be returned
   lda   BRK_SA         ; get stored accumulator
   rti                  ; return to calling code
_biosend:

BRK_65816N:
   .byte $E2,$30        ; SEP #$30
   sta   BRK_SA
                        ; TODO: prepare correctly
   bra   _isbrk         ; set to correct implementation

.segment "FIXEND"
_fixstart:
_unhandled:
   .byte $e2,$30        ; SEP #$30 -> set MX to 8-bit mode, like 65C02
   ldx   #UNH65816_IDX
   ldy   #UNH65816_BANK
   bra   bankgoto
NMI:
   jmp   (UVNMI)        ; user vector for NMI ($DF7A)
IRQ:
   jmp   (UVIRQ)        ; user vector for BRK/IRQ ($DF7E) -> irqcheck (default)
_reset:
   .byte $e2,$30        ; SEP #$30 -> set MX to 8-bit mode, like 65C02
   ldx   #RESET_IDX
   ldy   #KERNEL_BANK
bankgoto:
   sty   BANK           ; select bank from Y
   jmp   ($E001,x)      ; jump to table index, table will be used top down
_fixend:

.segment "VECTORS"
_vectors:
; $FFE0
.if 0
   .word $FFFF          ; reserved
   .word $FFFF          ; reserved
.endif
; $FFE4: (real) start of 65816 vectors
; per datasheet, vectors start at $FFE0, but first two are reserved
   .word _unhandled     ; 65816 native COP
   .word BRK_65816N     ; 65816 native BRK
   .word _unhandled     ; 65816 native ABORT (cannot be triggered)
   .word _unhandled     ; 65816 native NMI
   .word $FFFF          ; reserved           (cannot be triggered)
   .word _unhandled     ; 65816 native IRQ
; $FFF0
   .word $FFFF          ; reserved    (cannot be triggered)
   .word $FFFF          ; reserved    (cannot be triggered)
   .word _unhandled     ; 65816 COP
   .word $FFFF          ; reserved    (cannot be triggered)
   .word _unhandled     ; 65816 ABORT (cannot be triggered)

   .word NMI            ; 65C02 NMI
   .word RESET          ; 65C02 RESET
   .word IRQ            ; 65C02 IRQ

_biossize = (_biosend-BIOS)
_fixsize = (_fixend-_fixstart)
.out "   =============================="
.out .sprintf( "   BIOS size:  $%04x ($%04x free)", _biossize, $CA - (_biossize) )
.out .sprintf( "   FIXED size: $%04x", _fixsize )
.out "   =============================="

.assert  CHRIN      = _chrin,     error, "CHRIN at wrong address"
.assert  CHROUT     = _chrout,    error, "CHROUT at wrong address"
.assert  PRINT      = _print,     error, "PRINT at wrong address"
.assert  RESET      = _reset,     error, "RESET at wrong address"
.assert  _fixend    = _vectors,   error, "FIXEND segment not at end of memory"
