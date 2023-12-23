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

.include "native_kernel.inc"
.include "native_cpmfs.inc"

.global  cpmtests

copybios   := TRAMPOLINE

; set to 65c02 code
; ...best not make use of opcode that are not supported by 65816 CPUs
.PC02

reset:
   cld
   sei
   ldx   #$07
:
   lda   @vectab,x
   sta   UVBRK,x
   dex
   bpl   :-
   txs
   lda   #$00
   dec               ; 65C02 opcode that is a NOP in 6502
   bne   @iloop      ; no NMOS 6502, continue
@printerror:
   tax
@printchar:
   lda   @no65c02message,x
   beq   @printerror ; endless loop rePRINTing message
   jsr   chrout
   inx
   bne   @printchar
@no65c02message:
   .byte "65C02 required", 13, 0

@iloop:
   jsr   PRINT
   .byte 10,"Sorbus Native V", VERSION, ": 0-3)Boot, T)IM, W)ozMon? ", 0
:
   jsr   chrinuc
   bcs   :-
   cmp   #'0'
   bcc   :+
   cmp   #'4'
   bcc   @bootblock
:
   cmp   #'I'
   bne   :+
   lda   #<cpmtests
   ldx   #>cpmtests
   jsr   prhex16
   bne   @iloop
:
   cmp   #'R'
   bne   :+
   jmp   execram
:
   cmp   #'T'
   bne   :+
   jmp   timstart
:
.if 0
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
   .word brkhnd         ; UVBRK: IRQ handler dispatches BRK
   .word unmi           ; UVNMI: hardware NMI handler
   .word uirq           ; UVNBI: IRQ handler dispatches non-BRK
   .word irqcheck       ; UVIRQ: hardware IRQ handler

hirq:
   stz   TRAP
uirq:
   ;stz   TRAP
   lda   ASAVE
   jsr   PRINT
   .byte 10,"IRQ",10,0
   bit   TMICRL
   bmi   sbtimer
   rti

.if 0
ubrk:
   stz   TRAP
   lda   ASAVE
   jsr   PRINT
   .byte 10,"BRK",10,0
   rti
.endif

unmi:
   ;stz   TRAP
   jsr   PRINT
   .byte 10,"NMI",10,0
   bit   TMICRL
   bpl   notimer
sbtimer:
   jsr   PRINT
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

@notfound:
   jsr   PRINT
   .byte ": failed",10,0
   jmp   $E000

@checksig:
   dec   IDLBAL    ; reset LBA for re-reading to $E000
.if SECTOR_BUFFER <> $DF80
   ; when SECTOR_BUFFER is $DF80, then it's advanced to $E000 already
   stx   IDMEML    ; adjust target memory to
   lda   #$E0      ; $E000 following
   sta   IDMEMH
.endif

   ldx   #$04
:
   lda   SECTOR_BUFFER+3,x
   cmp   signature,x
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

execram:
   ldx   #(@trampolineend-@trampoline-1)
:
   lda   @trampoline,x
   sta   TRAMPOLINE,x
   dex
   bpl   :-
   jsr   PRINT
   .byte "Go",10,0
   jmp   TRAMPOLINE+@jmpbank0-@trampoline

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

signature:
   .byte "SBC23"
signatureend:

chrinuc:
   jsr   chrin
   bcs   chrinuc
uppercase:
   cmp   #'a'
   bcc   :+
   cmp   #'z'+1
   bcs   :+
   and   #$df
:
   rts

prhex16:                ; output 16 bit value in X,A
   pha
   txa
   jsr   prhex
   pla
   ; fall through
prhex:                  ; output 8 bit value in A
   pha                  ; save A for LSD
   lsr                  ; move MSD down to LSD
   lsr
   lsr
   lsr
   jsr   :+             ; print MSD
   pla                  ; restore A for LSD
   and   #$0f           ; mask LSD for hex PRINT
:
   ora   #'0'           ; add ascii "0"
   cmp   #':'           ; is still decimal
   bcc   :+             ; yes -> output
   adc   #$06           ; adjust offset for letters a-f
:
   jmp   chrout

brkhandler:
   stx   BRK_SX
   sty   BRK_SY
   tsx
   lda   $0102,x        ; get the return address from stack
   sta   TMP16+0
   lda   $0103,x
   sta   TMP16+1

   lda   TMP16+0        ; decrease it by one to get the BRK operand
   bne   :+
   dec   TMP16+1
:
   dec   TMP16+0
   lda   (TMP16)        ; finally get BRK operand!

   asl
   tax
   lda   @jumptable+0,x
   sta   TMP16+0
   lda   @jumptable+1,x
   sta   TMP16+1
   lda   BRK_SA
   ldx   BRK_SX
   jsr   @jump          ; jsr (indirect) is missing in opcodes :(

   ldy   BRK_SY
   ldx   BRK_SX
   jmp   brkdone
@jump:
   jmp   (TMP16)

@jumptable:
   .word prhex, prhex16
   .word cpmname, cpmload, cpmsave, cpmerase, cpmdir
   .word trap
trap:
   stx   $DF0C
   sty   $DF0D
   sta   $DF01
   rts

;-------------------------------------------------------------------------
; BIOS calls $FF00-$FFFF
;-------------------------------------------------------------------------

.segment "BIOS"
BIOS:
CHRIN:                  ; read character from UART
   jmp   chrin
CHROUT:                 ; write character to UART
   jmp   chrout
CHRCFG:                 ; set parameters for UART
   jmp   chrcfg
PRINT:                  ; print a string while keeping all registers
   sta   ASAVE          ; this routine changes A and P, so save those
   php
   pla
   sta   PSAVE
   pla                  ; get address of text from stack
   sta   TMP16+0
   pla
   sta   TMP16+1
@loop:
   inc   TMP16+0        ; since JSR stores return address - 1, start with
   bne   :+             ; ...incrementing the pointer
   inc   TMP16+1
:
   lda   (TMP16)        ; C02 opcode
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

chrin:
   lda   UARTRS         ; check for size of input queue
   bne   :+             ; data available, fetch it
   sec                  ; no input -> return 0 and set carry
   rts
:
   lda   UARTRD         ; get key value
   clc
   rts

chrout:
   bit   UARTWS         ; wait for buffer to accept data
   bmi   chrout
   sta   UARTWR         ; write data to output queue
   rts

chrcfg:
   bcs   @clear         ; carry on: set bits, off: clear bits
   ora   UARTCF
   bra   @store         ; bne would also work here
@clear:
   eor   #$ff
   and   UARTCF
@store:
   sta   UARTCF
   rts

RESET:
   lda   #$01
   sta   BANK
   jmp   reset
NMI:
   jmp   (UVNMI)
IRQ:
   jmp   (UVIRQ)

irqcheck:
   sta   BRK_SA
   pla
   pha
   and   #$10
   bne   :+
   lda   BRK_SA
   jmp   (UVNBI)
:
   lda   BRK_SA
   jmp   (UVBRK)

brkhnd:
   lda   BANK
   sta   BRK_SB
   lda   #$01
   sta   BANK
   jmp   brkhandler

brkdone:
   lda   BRK_SB
   sta   BANK
   lda   BRK_SA
   rti


.out "   ================"
.out .sprintf( "   BIOS size: $%04x", * - BIOS )
.out "   ================"

.segment "VECTORS"
   .word NMI
   .word RESET
   .word IRQ
