
.segment "CODE"
; BIOS ($E000-$E7FF)
.incbin "../../bin/cpm/CPM"

.align $800,$FF
; BDOS ($E800-$FEFF)
.incbin "../../bin/cpm/BDOS"

; $FF00-$FFFF is not used at all, because the kernel provides a function
; to copy $FF00-$FFFF to RAM. This area contains most basic functions
; like CHRIN, CHROUT, etc.
