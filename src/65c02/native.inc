
; Registers of Native Core I/O
;
; (V): vector
; (R): read-only
; (W): write-only
; (S): strobe, write value does not matter

; UART and miscellaneous stuff
BANK   = $DF00     ; ROM bank
TRAP   = $DF01     ; (S) output debug messages and reboot
RANDOM = $DF02     ; (R) get a random value

UARTCF = $DF0B     ; config: bit 0: turn on/off crlf conversion
UARTRD = $DF0C     ; (R) read from serial input queue (read only)
UARTRS = $DF0D     ; (R) get serial input queue size (read only, up to 127, >127: error)
UARTWR = $DF0E     ; (W) write to serial output queue (write only)
UARTWS = $DF0F     ; (R) get serial output queue size (read only, up to 127, >127: error)

TMBASE = $DF10
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

WDBASE = $DF20
WDOFF  = WDBASE+$0 ; stops timer
WDCL   = WDBASE+$1 ; set low counter, write resets watchdog when running
WDCM   = WDBASE+$2 ; set mid counter, write resets watchdog when running
WDCH   = WDBASE+$3 ; set high counter, stars watchdog, reset when running
; reading WDxxx indicates if watchdog is enabled ($80)

IRQSVB = $DF2C     ; PRELIMINARY!
BRK_SB = IRQSVB+$0 ; bank
BRK_SA = IRQSVB+$1 ; accumulator
BRK_SX = IRQSVB+$2 ; X index
BRK_SY = IRQSVB+$3 ; Y index
; this is just RAM space used by the kernel BRK routine

IDBASE = $DF70
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
