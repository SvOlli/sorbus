
.segment "CODE"
; BIOS ($E000-$E7FF)
.incbin "../bin/cpm/CPM"

.align $800,$FF
; BDOS ($E800-$FEFF)
.incbin "../bin/cpm/BDOS"

; $FF00-$FFFF is not used at all, because the kernel provides a function
; to copy $FF00-$FFFF to RAM. This area contains most basic functions
; like CHRIN, CHROUT, etc.

; however, a small stub if the copy of $FF00 isn't used
.if 0
.segment "BIOS"
NMI:
RESET:
IRQ:
   stx   $DF0C
   sty   $DF0D
   sta   $DF01          ; TRAP
   rti

.segment "VECTORS"
   .word NMI, RESET, IRQ
.endif

