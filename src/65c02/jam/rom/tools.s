
.import     browser
.import     gensine
.import     run6502asm
.import     unhandled65816


.segment "CODE"

dispatch:
   jmp   (@jmptab,x)
.assert dispatch = $E000, error, "tools dispatch must be at start of bank"

@jmptab:
   .word unhandled65816
   .word browser
   .word gensine
   .word run6502asm
