
; set processor to NMOS 6502
.p02

.include "../native.inc"
.include "../native_bios.inc"

;-------------------------------------------------------------------------
; BIOS calls $FF00-$FFFF
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
   bpl   @loop          ; get next char (65C02 only opcode, jmp also works)
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
   sta   TRAP
   rti                  ; return to calling code

.out "   =================================="
.out .sprintf( "   NMOS BIOS size: $%04x ($%04x free)", * - BIOS, $FA - (* - BIOS) )
.out "   =================================="

.segment "VECTORS"
NMI:
   jmp   (UVNMI)        ; user vector for NMI ($DF7A)
IRQ:
   jmp   (UVIRQ)        ; user vector for BRK/IRQ ($DF7E) -> irqcheck (default)
RESET:
   ; reset routine that also has to work when copied to RAM
   lda   #$01
   sta   BANK           ; set banking to first ROM bank (kernel)
   jmp   reset          ; jump to reset routine

   .word NMI
   .word RESET
   .word IRQ

.assert  CHRIN  = _chrin,  error, "CHRIN at wrong address"
.assert  CHROUT = _chrout, error, "CHROUT at wrong address"
.assert  PRINT  = _print,  error, "PRINT at wrong address"
