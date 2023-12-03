
; UART and miscellaneous stuff
UARTRD = $DFFA ; read from serial input queue (read only)
UARTRS = $DFFB ; get serial input queue size (read only, up to 127, >127: error)
UARTWR = $DFFC ; write to serial output queue (write only)
UARTWS = $DFFD ; get serial output queue size (read only, up to 127, >127: error)
RANDOM = $DFFE ; get a random value
BANK   = $DFFF ; ROM bank (not yet required)

TMBASE = $DF00
TMICRL = TMBASE + $0 ; repeating IRQ timer low, also stops timer
TMICRH = TMBASE + $1 ; repeating IRQ timer high, also starts timer
TMICSL = TMBASE + $2 ; single short IRQ timer low, also stops timer
TMICSH = TMBASE + $3 ; single short IRQ timer high, also starts timer
TMNCRL = TMBASE + $4 ; repeating NMI timer low, also stops timer
TMNCRH = TMBASE + $5 ; repeating NMI timer high, also starts timer
TMNCSL = TMBASE + $6 ; single short NMI timer low, also stops timer
TMNCSH = TMBASE + $7 ; single short NMI timer high, also starts timer

WDBASE = $DF08
WDOFF  = WDBASE+$0 ; stops timer
WDCL   = WDBASE+$1 ; set low counter, write resets watchdog when running
WDCM   = WDBASE+$2 ; set mid counter, write resets watchdog when running
WDCH   = WDBASE+$3 ; set high counter, stars watchdog, reset when running

IDBASE = $DF70
IDLBAL = IDBASE+$0 ; LBA low
IDLBAH = IDBASE+$1 ; LBA high
IDMEML = IDBASE+$2 ; DMA memory low
IDMEMH = IDBASE+$3 ; DMA memory high
IDREAD = IDBASE+$4 ; read sector (strobe, adjusts DMA memory and LBA)
IDWRT  = IDBASE+$5 ; write sector (strobe, adjusts DMA memory and LBA)
IDTRIM = IDBASE+$7 ; sector discard (strobe)
ID_LBA = IDBASE+$0 ; alternative notation: ID_LBA+0/ID_LBA+1
ID_MEM = IDBASE+$2 ; alternative notation: ID_MEM+0/ID_MEM+1 
