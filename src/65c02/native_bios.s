
.include "native.inc"
.include "native_bios.inc"

;-------------------------------------------------------------------------
; BIOS calls $FF00-$FFFF
;-------------------------------------------------------------------------

.segment "BIOS"
BIOS:
CHRIN:
   ; read character from UART
   jmp   chrin
CHROUT:
   ; write character to UART
   jmp   chrout
PRINT:
   ; print a string while keeping all registers
   sta   ASAVE          ; save A, it is used together with
   php                  ;   processor status register,
   pla                  ;   which can be only read via stack
   sta   PSAVE          ; save it
   pla                  ; get lobyte of address of text from stack and
   sta   TMP16+0        ;   store it into temporary vector
   pla                  ; same for hibyte
   sta   TMP16+1        ;   store
@loop:
   inc   TMP16+0        ; since JSR stores return address - 1, start with
   bne   :+             ;   incrementing the pointer
   inc   TMP16+1
:
   lda   (TMP16)        ; 65C02 only opcode, 6502 needs X=0 or Y=0
   beq   @out           ; $00 bytes indicates end of text
   jsr   chrout         ; output the character
   bra   @loop          ; get next char (65C02 only opcode, jmp also works)
@out:
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

RESET:
   ; reset routine that also works when copied to RAM
   lda   #$01
   sta   BANK           ; set banking to first ROM bank (kernel)
   jmp   reset          ; jump to reset routine
NMI:
   jmp   (UVNMI)        ; user vector for NMI ($DF7A)
IRQ:
   jmp   (UVIRQ)        ; user vector for BRK/IRQ ($DF7E) -> irqcheck (default)

IRQCHECK:
   sta   BRK_SA         ; let's figure out the source of the IRQ
   pla                  ; get processor status from stack
   pha                  ; and put it back
   and   #$10           ; check for BRK bit
   bne   @isbrk
   lda   BRK_SA         ; restore saved accumulator
   jmp   (UVNBI)        ; user vector for non-BRK IRQ ($DF7C)
@isbrk:
   lda   BANK           ; get current bank
   sta   BRK_SB         ; save it
   stx   BRK_SX
   sty   BRK_SY
   tsx
   lda   $0102,x        ; get the return address from stack
   sta   TMP16+0        ; and write it to temp vector
   lda   $0103,x        ; offset of current stack pointer is 1+2
   sta   TMP16+1        ; at offset 0 is processor status (got called via BRK)

   lda   TMP16+0        ; decrease it by one to get the BRK operand
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

   ldy   BRK_SY         ; get stored Y
   ldx   BRK_SX         ; get stored X
   lda   BRK_SB         ; get stored BANK
   sta   BANK           ; restore saved bank
   lda   BRK_SA         ; get stored accumulator
   rti                  ; return to calling code


.out "   ================"
.out .sprintf( "   BIOS size: $%04x", * - BIOS )
.out "   ================"

.segment "VECTORS"
   .word NMI
   .word RESET
   .word IRQ
