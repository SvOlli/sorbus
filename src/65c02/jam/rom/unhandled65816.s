
.include "jam.inc"
.include "jam_bios.inc"

.export     unhandled65816

unhandled65816:
   stz   TRAP
   jsr   PRINT
   .byte $0a,"unhandled 65816 vector triggered, did a trap will reset now."
   .byte $0a,$00
   jmp   RESET             ; must be RESET due to bank switch required
