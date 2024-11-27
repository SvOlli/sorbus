; IMPORTANT NOTE
; ==============
;
; This ROM for the native mode is in very early development.

.define VERSION "0.4"
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
; however, 6502 still should be capable of running WozMon

; NMOS 6502 compatible code start

reset:
   cld                  ; only required for "soft reset"
   sei                  ; no IRQs used in kernel

   ldx   #$07           ; move to BIOS code?
:
   lda   vectab,x
   sta   UVBRK,x        ; setup user vectors
   dex
   bpl   :-

   txs                  ; initialize stack to $FF

   lda   #$00
   ; multi purpose routine
   ; - set Z=$00 on 65CE02 in soft reset
   ; - detect 6502 from CMOS variants
   .byte $4b            ; 65CE02: taz -> make sure, sta (zp),z works like sta (zp)
                        ; 65C02: nop
                        ; 65816: phk (push program bank) -> like nop, but destroys byte on stack
                        ; 6502: ALR #im

   dec                  ; 65C02 opcode that on 6502 is an argument of $4b/ALR
   bne   cmos6502       ; no NMOS 6502, continue

   ; set NMI and IRQ to something more useful for NMOS 6502
   lda   #<nmosirq
   sta   UVNMI+0
   sta   UVIRQ+0
   lda   #>nmosirq
   sta   UVNMI+1
   sta   UVIRQ+1

@no65c02loop:
   jsr   PRINT
   .byte 13, "NMOS 6502 not supported, key to boot sector 2 ", 0
:
   iny
   bne   :-
   inx
   bne   :-
   jsr   CHRIN
   bcs   @no65c02loop

   lda   #$01
   jmp   boota

nmosirq:
   ;cld                  ; would be useful with NMOS, if doing more than return
   sta   TRAP
   rti

; NMOS 6502 compatible code end

cmos6502:
   txs                  ; fix for 65816's phk
@iloop:
   jsr   PRINT          ;       2         3         4         5         6         7         8
   ;         12345678901234567890123456789012345678901234567890123456789012345678901234567890
   .byte 10,"Sorbus Native V", VERSION
   .byte                       ": 1-4)Bootsector, 0)Exec RAM @ $E000,"
   .byte 10,"F)ilebrowser, B)ASIC, System M)onitor, T)IM, W)ozMon?",10,0
:
   jsr   chrinuc        ; wait for keypress and make it uppercase
   bcs   :-

   ldx   #<(@jmps-@keys-1)
:
   cmp   @keys,x
   beq   @found
   dex
   bpl   :-
   jmp   @iloop
@found:
   pha                  ; save selected option by input
   txa
   asl
   tax
   pla
   jmp   (@jmps,x)

@keys:
   .byte "01234BFIMTW"
@jmps:
   .word execram        ; 0
   .word boot           ; 1
   .word boot           ; 2
   .word boot           ; 3
   .word boot           ; 4
   .word basic          ; B
   .word filebrowser    ; F
   .word @info          ; I
   .word mon_init       ; M
   .word timstart       ; T
   .word woz            ; W

@info:
   lda   #$0a
   jsr   CHROUT

:
   lda   TRAP           ; find start of data stream after $00 byte
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

boot:
   dec
boota:
   ; boottrack is 32k, memory at $E000 is 8k
   ; starting at boota can load other 8k offsets from internal drive
   ; this code can be run by NMOS 6502
   jsr   PRINT ; preserves all registers... spooky.
   .byte 10,"Checking bootblock ",0
   and   #$03
   pha
   clc                  ; add one extra for bootblock starting with 1
   adc   #$01
   jsr   prhex4
   pla
   clc
   ror
   ror
   ror
   sta   ID_LBA+0
   lda   #$00
   sta   ID_LBA+1
   lda   #<SECTOR_BUFFER
   sta   ID_MEM+0
   lda   #>SECTOR_BUFFER
   sta   ID_MEM+1

   sta   IDREAD

   lda   IDREAD
   bne   bootfailed

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
   bne   bootfailed     ; throw error if not
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
   bra   execram

xgensine:
   ldx   #$02           ; tools bank starts with jmp ($e003,x)
   .byte $2c
filebrowser:
   ldx   #$00           ; tools bank starts with jmp ($e003,x)
   lda   #$02           ; select tools bank
   .byte $2c
basic:
   lda   #$03           ; select BASIC bank
   .byte $2c
execram:
   ; execute loaded boot block in RAM at $E000
   lda   #$00
execrom:                ; has to be called with A=bank to switch to
   pha
   ldy   #(@trampolineend-@trampoline-1)
:
   lda   @trampoline,y  ; this requires bankswitching code written to RAM
   sta   TRAMPOLINE,y
   dey
   bpl   :-
   pla
   jmp   TRAMPOLINE+@jmpbank0-@trampoline

@trampoline:
   inc   BANK           ; this routine may only be called from bank 0 (RAM)
   jsr   copybios       ; copy $FF00-$FFFF to RAM
   stz   BANK           ; set BANK back to $00 (RAM)
   rts
@jmpbank0:
   sta   BANK
   jmp   $E000
@trampolineend:

bootfailed:
   jsr   PRINT
   .byte ": failed, dropping to WozMon",10,0

woz:
   lda   #$0a           ; start WozMon port
   jsr   CHROUT
:  ; workaround for WozMon not handling RTS when executing external code
   jsr   wozstart
   ; will be reached if own code run within WozMon exits using rts
   jmp   :-             ; no bra here: NMOS 6502 fallback mode


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
   cmp   #<((@jumptableend - @jumptable)/2)
   bcc   @okay          ; sanity check: if BRK operand out of scope
   lda   #$00           ; reset, user BRK can lda (TMP16) to get BRK operand
@okay:
   asl                  ; BRK >=$80 will always be =$00
   tax
   lda   @jumptable+1,x ; get address from jump table
   pha
   lda   @jumptable+0,x
   pha
   tsx
   lda   $0105,x        ; get processor state from stack
   pha                  ; and save it for rti below
   ldx   BRK_SX         ; get stored X
   lda   BRK_SA         ; get stored A
   rti                  ; jump to handling subroutine

@user:
   jmp   (UVBRK)

@jumptable:
   .word @user, chrinuc, chrcfg, prhex8, prhex16         ; $00-$04
   .word cpmname, cpmload, cpmsave, cpmerase, cpmdir     ; $05-$09
   .word vt100, copybios, xinputline, xgensine, mon_brk  ; $0a-$0e
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

xinputline:
   bit   BRK_SY
   bmi   :+
   jmp   inputline
:
   jmp   inputline      ; replace this with inputlinecfg, once implemented


.segment "DATA"
vectab:
   .word mon_brk        ; UVBRK: IRQ handler dispatches BRK
   .word mon_nmi        ; UVNMI: hardware NMI handler
   .word mon_irq        ; UVNBI: IRQ handler dispatches non-BRK
   .word IRQCHECK       ; UVIRQ: hardware IRQ handler
