
.include "../native_bios.inc"

; these addresses are not really required, since those are only executed
; in bank 1 (kernel), but this stub is for every other bank.

reset    = $e000
brkjump  = $e19e
