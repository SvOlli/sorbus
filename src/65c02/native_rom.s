; IMPORTANT NOTE
; ==============
;
; This ROM for the native mode is in very early development.

.define VERSION "0.1"
.segment "CODE"

; zeropage addresses used by this part of ROM

ASAVE := $FC
PSAVE := $FD
TMP16 := $FE

SECTOR_BUFFER := $DF80
TRAMPOLINE    := $0100

.include "native_rom.inc"

copybios   := TRAMPOLINE

; set to 65c02 code
; ...best not make use of opcode that are not supported by 65816 CPUs
.PC02

;-------------------------------------------------------------------------
; jumptable
;-------------------------------------------------------------------------

reset:
   cld
   sei
   ldx   #$05
:
   lda   @vectab,x
   sta   UVNMI,x
   dex
   bpl   :-
   txs
   lda   #$01
   dec               ; 65C02 opcode that is a NOP in 6502
   beq   @iloop      ; no NMOS 6502, continue
@printerror:
   ldx   #$00
@printchar:
   lda   @no65c02message,x
   jsr   chrout
   iny
   bne   @printchar
   beq   @printerror ; endless loop reprinting message
@no65c02message:
   .byte "65C02 required", 10, 0

@iloop:
   jsr   PRINT
   .byte 10,"Sorbus Native V", VERSION, ": 0-3)Boot, T)IM, W)ozMon? ", 0
:
   jsr   chrin
   bcs   :-
   jsr   uppercase
   cmp   #'0'
   bcc   :+
   cmp   #'4'
   bcc   @bootblock
:
   cmp   #'T'
   bne   :+
   jmp   timstart
:
.if 1
   ; for speed measurement
   cmp   #'S'
   beq   :-
.endif
   cmp   #'W'
   bne   @iloop
   lda   #$0a
   jsr   chrout
:  ; workaround for WozMon not handling RTS when executing external code
   jsr   wozstart
   bra   :-
@bootblock:
   and   #$03
   jmp   boot+2

@vectab:
   .word unmi
   .word ubrk
   .word uirq

uirq:
   ;stz   TRAP
   sta   ASAVE
   pla
   pha
   and   #$10
   bne   ubrk
   lda   ASAVE
   jsr   print
   .byte 10,"IRQ",10,0
   bit   TMICRL
   bmi   sbtimer
   rti

ubrk:
   ;stz   TRAP
   lda   ASAVE
   jsr   print
   .byte 10,"BRK",10,0
   rti

unmi:
   ;stz   TRAP
   jsr   print
   .byte 10,"NMI",10,0
   bit   TMICRL
   bpl   notimer
sbtimer:
   jsr   print
   .byte "triggered by sorbus timer",10,0
notimer:
   rti

todo:
   stz   TRAP

bootx:
   lda   $0f
   .byte $2c   ; skip following 2-byte instruction
boot:
   lda   #$00
   ; boottrack is 32k, memory at $E000 is 8k
   ; starting at boot+2 can load other 8k offsets from internal drive
   jsr   PRINT ; preserves all registers... spooky.
   .byte 10,"Checking bootblock ",0
   and   #$03
   pha
   ora   #'0'
   jsr   CHROUT
   pla
   clc
   ror
   ror
   ror
   sta   IDLBAL
   ldx   #$00
   stx   IDLBAH
.if (<SECTOR_BUFFER <> $00)
   lda   #<SECTOR_BUFFER
   sta   IDMEML
.else
   stx   IDMEML
.endif
   lda   #>SECTOR_BUFFER
   sta   IDMEMH

   sta   IDREAD

   lda   IDREAD
   beq   @checksig
   jsr   PRINT
   .byte ": no internal drive",0
   bra   @errend

@notfound:
   jsr   PRINT
   .byte ": no boot signature",0
@errend:
   jsr   PRINT
   .byte " found",10,0
   jmp   $E000

@checksig:
   dec   IDLBAL    ; reset LBA for re-reading to $E000
   stx   IDMEML    ; adjust target memory to
   lda   #$E0      ; $E000 following
   sta   IDMEMH

   ldx   #$04
:
   lda   SECTOR_BUFFER+3,x
   cmp   @signature,x
   bne   @notfound
   dex
   bpl   :-

   jsr   PRINT
   .byte ": found.",10,"Loading... ",0

   ldy   #$40        ; banksize ($2000) / sectorsize ($80)
:
   sta   IDREAD
   dey
   bne   :-
   ; leave with Y = 0, as required in @jmpcode

   ldx   #(@trampolineend-@trampoline-1)
:
   lda   @trampoline,x
   sta   TRAMPOLINE,x
   dex
   bpl   :-
   jsr   PRINT
   .byte "Go",10,0
   jmp   TRAMPOLINE+@jmpbank0-@trampoline

@signature:
   .byte "SBC23"
@signatureend:

@trampoline:
   inc   BANK              ; this routine will only be called from bank 0 (RAM)
   jsr   @copybios
   stz   BANK              ; set BANK back to $00 (RAM)
   rts
@jmpbank0:
   stz   BANK      ; Y = 0
   jmp   $E000
@trampolineend:

@copybios:
   ldx   #$00
:
   lda   BIOS,x
   sta   BIOS,x
   inx
   bne   :-
   rts

uppercase:
   cmp   #'a'
   bcc   :+
   cmp   #'z'
   bcs   :+
   and   #$df
:
   rts

prhex:
   pha            ; save A for LSD
   lsr            ; move MSD down to LSD
   lsr
   lsr
   lsr
   jsr   :+       ; print MSD
   pla            ; restore A for LSD
   and   #$0f     ; mask LSD for hex print
:
   ora   #'0'     ; add ascii "0"
   cmp   #':'     ; is still decimal
   bcc   :+       ; yes -> output
   adc   #$06     ; adjust offset for letters a-f
:
   jmp   chrout


bank1jsr:
   php
   pha
   phx
   phy
   tsx
   lda   $0103,x     ; find address lobyte of payload address
   sta   TMP16+0     ; save it
   clc
   adc   #$02        ; skip two bytes on return address (payload)
   sta   $0103,x
   lda   $0104,x     ; find address hibyte of payload address
   sta   TMP16+1     ; save it
   bcc   :+
   inc   $0104,x     ; adjust hibyte of return address, if required
:
   ldy   #$01        ; jsr stores return address - 1 on stack, compensate
   lda   (TMP16),y   ; get lobyte of payload (target address)
   pha
   iny
   lda   (TMP16),y   ; get hibyte of payload (target address)
   sta   TMP16+1     ; reuse vector for jump address
   pla
   sta   TMP16+0
   ply
   plx
   pla
   plp
   jmp   (TMP16)

.segment "BIOS"
BIOS:
CHRIN:               ; reading character from UART
   jmp   chrin
CHROUT:              ; writing character to UART
   jmp   chrout
CHRCFG:              ; setting parameters for UART
   jmp   chrcfg
PRINT:               ; print a string while keeping all registers
   jmp   print
BANK1JSR:            ; from RAM, jump to ROM bank 1 to a subroutine
   php
   pha
   lda   BANK
   sta   PSAVE
   lda   #$01
   sta   BANK
   pla
   plp
   jsr   bank1jsr
   php
   pha
   lda   PSAVE
   sta   BANK
   pla
   plp
   rts

chrin:
   lda   UARTRS      ; check for size of input queue
   bne   :+          ; data available, fetch it
   sec               ; no input -> return 0 and set carry
   rts
:
   lda   UARTRD      ; get key value
   clc
   rts

chrout:
   bit   UARTWS      ; wait for buffer to accept data
   bmi   chrout
   sta   UARTWR      ; write data to output queue
   rts

chrcfg:
   bcs   @clear      ; carry on: set bits, off: clear bits
   ora   UARTCF
   bra   @store      ; bne would also work here
@clear:
   eor   #$ff
   and   UARTCF
@store:
   sta   UARTCF
   rts

print:
   sta   ASAVE       ; this routine changes A and P, so save those
   php
   pla
   sta   PSAVE
   pla               ; get address of text from stack
   sta   TMP16+0
   pla
   sta   TMP16+1
@loop:
   inc   TMP16+0     ; since JSR stores return address - 1, start with
   bne   :+          ; ...incrementing the pointer
   inc   TMP16+1
:
   lda   (TMP16)     ; C02 opcode
   beq   @out
   jsr   chrout
   bra   @loop
@out:
   lda   TMP16+1
   pha
   lda   TMP16+0
   pha
   lda   PSAVE
   pha
   lda   ASAVE
   plp
   rts

RESET:
   lda   #$01
   sta   BANK
   jmp   reset
NMI:
   jmp   (UVNMI)
IRQ:
   jmp   (UVIRQ)

.segment "VECTORS"
   .word NMI
   .word RESET
   .word IRQ
