
.segment "CODE"
; BIOS ($E000-$E7FF)
.incbin "../bin/cpm/sorbus.bin"

.align $800,$FF
; BDOS ($E800-$FEFF)
.incbin "../bin/cpm/bdos.sys"

; $FF00-$FFFF is not used at all, because the kernel provides a function
; to copy $FF00-$FFFF to RAM. This area contains most basic functions
; like CHRIN, CHROUT, etc.

; however, a small stub if the copy of $FF00 isn't used
.segment "BIOS"
NMI:
RESET:
IRQ:
   rti

.segment "VECTORS"
   .word NMI, RESET, IRQ
