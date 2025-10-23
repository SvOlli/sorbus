
.import     browser
.import     gensine
.import     run6502asm


.segment "CODE"

dispatch:
   jmp   (@jmptab,x)

.assert dispatch = $E000, error, "tools dispatch must be at start of bank"

@jmptab:
   .word browser
   .word gensine
   .word run6502asm

