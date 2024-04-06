; IMPORTANT NOTE
; ==============
;
; This ROM for the native mode is in very early development.

.define VERSION "0.3"
.segment "CODE"

; zeropage addresses used by this part of ROM


SECTOR_BUFFER := $DF80
TRAMPOLINE    := $0100

.include "../native.inc"
.include "../native_bios.inc"
.include "../native_kernel.inc"
.include "../native_cpmfs.inc"

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
@no65c02loop:
   tax
@no65c02char:
   lda   @no65c02message,x
   beq   @no65c02loop   ; endless loop reprinting message
   jsr   CHROUT
   inx
   jmp   @no65c02char

@no65c02message:
   .byte "65C02 required", 13, 0

@vectab:
   .word brktrap        ; UVBRK: IRQ handler dispatches BRK
   .word unmi           ; UVNMI: hardware NMI handler
   .word uirq           ; UVNBI: IRQ handler dispatches non-BRK
   .word IRQCHECK       ; UVIRQ: hardware IRQ handler

@iloop:
   jsr   PRINT
   .byte 10,"Sorbus Native V", VERSION
   .byte ": 0-3)Boot, F)ilebrowser, B)ASIC, T)IM, W)ozMon? ", 0
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
   cmp   #'B'
   bne   :+
   ldy   #$03
@execrom2:
   jmp   execrom        ; execute 2nd ROM bank @ $E000
:
   cmp   #'F'
   bne   :+
   ldy   #$02
   ldx   #$00           ; jmp vector 0: file browser
   bra   @execrom2      ; execute 2nd ROM bank @ $E000
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
   beq   @info

   cmp   #'W'
   bne   @iloop2

   lda   #$0a           ; start WozMon port
   jsr   CHROUT
:  ; workaround for WozMon not handling RTS when executing external code
   jsr   wozstart
   bra   :-
@bootblock:
   and   #$03
   jmp   boota

@info:
   lda   #$0a
   jsr   CHROUT

:
   lda   TRAP
   bne   :-
:
   lda   TRAP
   bne   @out
@iloop2:
   jmp   @iloop
@out:
   jsr   prhex8
   lda   #' '
   jsr   CHROUT
   bra   :-

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

   jsr   PRINT
   .byte "Go",10,0
execram:
   ; execute loaded boot block in RAM at $E000
   ldy   #$00
execrom:                ; has to be called with Y=bank to switch to
   phy
   ldy   #(@trampolineend-@trampoline-1)
:
   lda   @trampoline,y  ; this requires bankswitching code written to RAM
   sta   TRAMPOLINE,y
   dey
   bpl   :-
   ply
   jmp   TRAMPOLINE+@jmpbank0-@trampoline

@trampoline:
   inc   BANK           ; this routine may only be called from bank 0 (RAM)
   jsr   copybios       ; copy $FF00-$FFFF to RAM
   stz   BANK           ; set BANK back to $00 (RAM)
   rts
@jmpbank0:
   sty   BANK
   jmp   $E000
@trampolineend:

copybios:
   ; copy the BIOS page to RAM
   ldx   #$00
:
   lda   BIOS,x
   sta   BIOS,x
   inx
   bne   :-
   rts

brkjump:
   asl                  ; make it word offset
   bcs   @overflow      ; sanity check: BRK operand >= $80
   cmp   #<(@jumptableend - @jumptable)
   bcc   @okay          ; sanity check: if BRK operand out of scope
@overflow:
   lda   #$00           ; reset, user BRK can lda (TMP16) to get BRK operand
@okay:
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
   .word vt100, copybios, inputline, gensine
@jumptableend:

signature:
   .byte "SBC23"
signatureend:

chrinuc:
   ; wait for character from UART and make it uppercase
   jsr   CHRIN
   bcs   chrinuc
   jsr   uppercase
   sta   BRK_SA         ; when called via BRK, this is required
   rts

uppercase:
   cmp   #'a'
   bcc   :+
   cmp   #'z'+1
   bcs   :+
   and   #$df
:
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
   jmp   CHROUT

brktrap:
   ; make some stuff visible for backtrace
   cmp   (TMP16)        ; read BRK operand
   stx   UARTRD         ; store X (read-only I/O register $DF0C)
   sty   UARTRS         ; store Y (read-only I/O register $DF0D)
   sta   TRAP           ; store A (TRAP strobe register   $DF01)
   rts                  ; return, if system continues after TRAP
