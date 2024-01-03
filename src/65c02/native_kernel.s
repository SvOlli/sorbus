; IMPORTANT NOTE
; ==============
;
; This ROM for the native mode is in very early development.

.define VERSION "0.2"
.segment "CODE"

; zeropage addresses used by this part of ROM

ASAVE := $06
PSAVE := $05
TMP16 := $04

SECTOR_BUFFER := $DF80
TRAMPOLINE    := $0100

.include "native.inc"
.include "native_bios.inc"
.include "native_kernel.inc"
.include "native_cpmfs.inc"

copybios   := TRAMPOLINE

; set to 65c02 code
; ...best not make use of opcode that are not supported by 65816 CPUs
.PC02

; reasons to use 65C02 instead of NMOS:
; - CPU can be stopped
; - fixes:
;   - D flag after interrupt
;   - jmp ($xxFF)
;   - BRK and IRQ at the same time
; - opcodes:
;   - phx, phy, plx, ply
;   - lda (zp), ora (zp)
;   - stz

reset:
   cld                  ; only required for "soft reset"
   sei                  ; no IRQs used in kernel
   ldx   #$07
:
   lda   @vectab,x
   sta   UVBRK,x        ; setup user vectors
   dex
   bpl   :-

   txs                  ; initialize stack to $FF
   lda   #$00           ; this kernel uses 65C02 opcodes, throw error on NMOS
   dec                  ; 65C02 opcode that is a NOP in 6502
   bne   @iloop         ; no NMOS 6502, continue
@printerror:
   tax
@printchar:
   lda   @no65c02message,x
   beq   @printerror    ; endless loop reprinting message
   jsr   chrout
   inx
   bne   @printchar
@no65c02message:
   .byte "65C02 required", 13, 0

@iloop:
   jsr   PRINT
   .byte 10,"Sorbus Native V", VERSION, ": 0-3)Boot, T)IM, W)ozMon? ", 0
:
   jsr   chrinuc        ; wait for keypress and make it uppercase
   bcs   :-
   cmp   #'0'           ; is it a boot block number?
   bcc   :+
   cmp   #'4'
   bcc   @bootblock
:
   cmp   #'R'           ; hidden CP/M debugging feature
   bne   :+
   jmp   execram        ; execute RAM bank @ $E000
:
   cmp   #'T'
   bne   :+
   jmp   timstart       ; start 6530-004 (TIM) monitor port
:
.if 0
   ; for speed measurement, debug only
   cmp   #'S'
   beq   :-
.endif

   cmp   #'I'           ; developer info: print out core info string
   bne   @noinfo

   lda   #$0a
   jsr   CHROUT

:
   lda   TRAP
   bne   :-
:
   lda   TRAP
   beq   @iloop
   jsr   prhex8
   lda   #' '
   jsr   CHROUT
   bra   :-

@noinfo:
   cmp   #'W'
   bne   @iloop

   lda   #$0a           ; start WozMon port
   jsr   CHROUT
:  ; workaround for WozMon not handling RTS when executing external code
   jsr   wozstart
   bra   :-
@bootblock:
   and   #$03
   jmp   boota

@vectab:
   .word brktrap        ; UVBRK: IRQ handler dispatches BRK
   .word unmi           ; UVNMI: hardware NMI handler
   .word uirq           ; UVNBI: IRQ handler dispatches non-BRK
   .word irqcheck       ; UVIRQ: hardware IRQ handler

uirq:
   ; dummy IRQ handler: prints out IRQ and checks source
   ;stz   TRAP
   jsr   PRINT
   .byte 10,"IRQ",10,0
   bit   TMICRL
   bmi   sbtimer
   rti

unmi:
   ; dummy NMI handler: prints out IRQ and checks source
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

boot:
   lda   #$00           ; load first boot block (not used since user input)
boota:
   ; boottrack is 32k, memory at $E000 is 8k
   ; starting at boota can load other 8k offsets from internal drive
   jsr   PRINT ; preserves all registers... spooky.
   .byte 10,"Checking bootblock ",0
   and   #$03
   pha
   jsr   prhex4
   pla
   clc
   ror
   ror
   ror
   sta   ID_LBA+0
   stz   ID_LBA+1
.if (<SECTOR_BUFFER <> $00)
   lda   #<SECTOR_BUFFER
   sta   ID_MEM+0
.else
   stz   ID_MEM+0
.endif
   lda   #>SECTOR_BUFFER
   sta   ID_MEM+1

   sta   IDREAD

   lda   IDREAD
   beq   @checksig

@notfound:
   jsr   PRINT
   .byte ": failed",10,0
   jmp   $E000

@checksig:
   dec   ID_LBA+0       ; reset LBA for re-reading to $E000
.if SECTOR_BUFFER <> $DF80
   ; when SECTOR_BUFFER is $DF80, then it's advanced to $E000 already
   stz   ID_MEM+0       ; adjust target memory to
   lda   #$E0           ; $E000 following
   sta   ID_MEM+1
.endif

   ldx   #$04
:
   lda   SECTOR_BUFFER+3,x
   cmp   signature,x    ; verify if signature matches
   bne   @notfound      ; throw error if not
   dex
   bpl   :-

   jsr   PRINT
   .byte ": found.",10,"Loading... ",0

   ldy   #$40           ; banksize ($2000) / sectorsize ($80)
:
   sta   IDREAD         ; yep, it's really that easy to load a boot block
   dey
   bne   :-

execram:
   ; execute loaded boot block in RAM at $E000
   ldx   #(@trampolineend-@trampoline-1)
:
   lda   @trampoline,x  ; this requires bankswitching code written to RAM
   sta   TRAMPOLINE,x
   dex
   bpl   :-
   jsr   PRINT
   .byte "Go",10,0
   jmp   TRAMPOLINE+@jmpbank0-@trampoline

@trampoline:
   inc   BANK           ; this routine may only be called from bank 0 (RAM)
   jsr   @copybios      ; copy $FF00-$FFFF to RAM
   stz   BANK           ; set BANK back to $00 (RAM)
   rts
@jmpbank0:
   stz   BANK
   jmp   $E000
@trampolineend:

@copybios:
   ; copy the BIOS page to RAM
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
   ; wait for character from UART and make it uppercase
   jsr   chrin
   bcs   chrinuc
uppercase:
   cmp   #'a'
   bcc   :+
   cmp   #'z'+1
   bcs   :+
   and   #$df
:
   sta   BRK_SA         ; when called via BRK, this is required
   rts

chrcfg:
   ; this function manipulates the configuration bits of CHRIN/CHROUT
   ; A = bits to set
   ; X = bits to clear
   ; return previous value is return in A
   pha                  ; write bits to set
   lda   UARTCF         ; get configration value
   sta   BRK_SA         ; store it for return
   txa                  ; now get bits to clean
   eor   #$ff           ; invert them
   and   UARTCF         ; clear them
   tsx
   ora   $0101,x        ; set bits directly from stack
   sta   UARTCF         ; write configuration register
   pla                  ; drop value from stack
   rts

prhex16:
   ; output 16 bit value in X,A
   pha
   txa
   jsr   prhex8
   pla
   ; fall through
prhex8:
   ; output 8 bit value in A
   pha                  ; save A for LSD
   lsr                  ; move MSD down to LSD
   lsr
   lsr
   lsr
   jsr   prhex4         ; print MSD
   pla                  ; restore A for LSD
prhex4:
   and   #$0f           ; mask LSD for hex PRINT
   ora   #'0'           ; add ascii "0"
   cmp   #':'           ; is still decimal
   bcc   :+             ; yes -> output
   adc   #$06           ; adjust offset for letters A-F
:
   jmp   chrout

brkjump:
   asl                  ; make it word offset
   cmp   #<(@jumptableend - @jumptable)
   bcc   :+             ; sanity check: if BRK operand out of scope
   lda   #$00           ; reset, user BRK can lda (TMP16) to get BRK operand
:
   tax
   lda   @jumptable+1,x ; get address from jump table
   pha
   lda   @jumptable+0,x
   pha
   tsx
   lda   $0105,x        ; get processor state from stack
   pha                  ; and save it for later rti
   ldx   BRK_SX         ; get stored X
   lda   BRK_SA         ; get stored A
   rti                  ; jump to handling subroutine

@user:
   jmp   (UVBRK)

@jumptable:
   .word @user, chrinuc, chrcfg, prhex8, prhex16
   .word cpmname, cpmload, cpmsave, cpmerase, cpmdir
@jumptableend:

brktrap:
   ; make some stuff visible for backtrace
   cmp   (TMP16)        ; read BRK operand
   stx   UARTRD         ; store X (read-only I/O register $DF0C)
   sty   UARTRS         ; store Y (read-only I/O register $DF0D)
   sta   TRAP           ; store A (TRAP strobe register   $DF01)
   rts                  ; return, if system continues after TRAP


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

irqcheck:
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
