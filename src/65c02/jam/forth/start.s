
.include "jam.inc"
.include "jam_bios.inc"

start:
   jsr   PRINT
   .byte 10,"Coming soon: some Forth implementation...", 10, 0
   jmp   ($FFFC)

