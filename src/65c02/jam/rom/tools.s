
.include "jumptable.inc"

.import     browser
.import     gensine
.import     run6502asm
.import     unhandled65816


.segment "CODE"

.assert * = B2UNH816, error, "B2UNH816 does not match"
   jmp   unhandled65816
.assert * = B2GENSINE, error, "B2GENSINE does not match"
   jmp   gensine
.assert * = B2BROWSER, error, "B2BROWSER does not match"
   jmp   browser
.assert * = B2RUN6502ASM, error, "B2RUN6502ASM does not match"
   jmp   run6502asm
