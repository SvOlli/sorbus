;
; Copyright 2023-2024 Sven Oliver ("SvOlli") Moll
; Distributed unter the terms: GPL-3.0-or-later
;

; Registers of JAM Core I/O
;
; (V): vector
; (R): read-only
; (W): write-only
; (S): strobe, write value does not matter

; used zeropage addresses
ASAVE  = $07
PSAVE  = $06
TMP16  = $04

; UART and miscellaneous stuff
BANK   = $DF00     ; ROM bank
SBCID  = $DF01     ; (R) version information
TRAP   = $DF01     ; (S) output debug messages and drop to meta mode
RANDOM = $DF02     ; (R) get a random value
XRAMSW = $DF03     ; (W) save/restore/swap pages $00-$03
                   ; bits 0-3 -> pages 0-3 affected
                   ; $40 RAM -> buffer, $80 RAM <- buffer, $c0 RAM <-> buffer
CPUID  = $DF04     ; (R) CPU features:
                   ; $01 NMOS, $02 CMOS, $04 BIT (RE)SET, $08 Z reg, $10 16 bit
                   ; known CPUs:
                   ; $01 6502, $02 65SC02, $06 65C02, $0E 65CE02, $12 65816
                   ; $21 6502 Rev.A (no ROR)
UARTCF = $DF0B     ; config: bit 0: turn on/off crlf conversion
                   ;         bit 1: turn on/off flow control
UARTRD = $DF0C     ; (R) read from serial input queue (read only)
UARTRS = $DF0D     ; (R) get serial input queue size (read only, up to 127, >127: error)
UARTWR = $DF0E     ; (W) write to serial output queue (write only)
UARTWS = $DF0F     ; (R) get serial output queue size (read only, up to 127, >127: error)

TMBASE = $DF10     ; timers
TMICRL = TMBASE+$0 ; repeating IRQ timer low, also stops timer
TMICRH = TMBASE+$1 ; repeating IRQ timer high, also starts timer
TMICSL = TMBASE+$2 ; single short IRQ timer low, also stops timer
TMICSH = TMBASE+$3 ; single short IRQ timer high, also starts timer
; reading TMIxxx indicates if IRQ was triggered by timer ($80), also clears
TMNCRL = TMBASE+$4 ; repeating NMI timer low, also stops timer
TMNCRH = TMBASE+$5 ; repeating NMI timer high, also starts timer
TMNCSL = TMBASE+$6 ; single short NMI timer low, also stops timer
TMNCSH = TMBASE+$7 ; single short NMI timer high, also starts timer
; reading TMNxxx indicates if NMI was triggered by timer ($80), also clears
TMIMRL = TMBASE+$8 ; repeating IRQ timer low
TMIMRH = TMBASE+$9 ; repeating IRQ timer high, both $00 stop timer
TMNMRL = TMBASE+$A ; repeating NMI timer low
TMNMRH = TMBASE+$B ; repeating NMI timer high, both $00 stop timer

WDBASE = $DF20     ; watchdog
WDOFF  = WDBASE+$0 ; stops timer
WDCL   = WDBASE+$1 ; set low counter, write resets watchdog when running
WDCM   = WDBASE+$2 ; set mid counter, write resets watchdog when running
WDCH   = WDBASE+$3 ; set high counter, stars watchdog, reset when running
; reading WDxxx indicates if watchdog is enabled ($80)

CYCNTR = $DF24     ; PRELIMINARY!
; read only 32 bit value little endian up to $DF27
; reading $DF24 copies value to shadow register from which is read

IRQSVB = $DF2C     ; shadow registers for IRQ/BRK handling PRELIMINARY!
BRK_SB = IRQSVB+$0 ; bank
BRK_SA = IRQSVB+$1 ; accumulator
BRK_SX = IRQSVB+$2 ; X index
BRK_SY = IRQSVB+$3 ; Y index
; this is just RAM space used by the kernel BRK routine

MONREG = $DF30     ; system monitor shadow registers
MON_PC = MONREG+$0 ; program counter
MON_BK = MONREG+$2 ; bank (shadow of $DF01)
MON_A  = MONREG+$3 ; accumulator
MON_X  = MONREG+$4 ; X register
MON_Y  = MONREG+$5 ; Y register
MON_SP = MONREG+$6 ; stack pointer
MON_P  = MONREG+$7 ; processor status

IDBASE = $DF70     ; internal drive
IDLBAL = IDBASE+$0 ; LBA low
IDLBAH = IDBASE+$1 ; LBA high
ID_LBA = IDBASE+$0 ; alternative notation: ID_LBA+0/ID_LBA+1
IDMEML = IDBASE+$2 ; DMA memory low
IDMEMH = IDBASE+$3 ; DMA memory high
ID_MEM = IDBASE+$2 ; alternative notation: ID_MEM+0/ID_MEM+1
IDREAD = IDBASE+$4 ; (S) read sector (adjusts DMA memory and LBA)
                   ; (R) error code from last read ($00=ok)
IDWRT  = IDBASE+$5 ; (S) write sector (adjusts DMA memory and LBA)
                   ; (R) error code from last write ($00=ok)
IDTRIM = IDBASE+$7 ; sector discard (strobe)

UVBRK  = $DF78     ; if UVIRQ handles BRK, vector to BRK $00 routine
UVNMI  = $DF7A     ; NMI does JMP (UVNMI)
UVNBI  = $DF7C     ; if UVIRQ handles BRK, vector to non-BRK routine
UVIRQ  = $DF7E     ; IRQ does JMP (UVIRQ)
