
.include "jam_bios.inc"

; These addresses are not really required, since those are only accessed
; after switching to bank 1 for kernel. This stub is for every other bank.

.export     reset
.export     brkjump
.export     _mon_nmi

reset    = $e000
brkjump  = $ffda
_mon_nmi = $e003
