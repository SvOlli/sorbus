
.import     browser
.import     gensine


.segment "CODE"

dispatch:
   jmp   (@jmptab,x)

.assert dispatch = $E000, error, "tools dispatch must be at start of bank"

@jmptab:
   .word browser
   .word gensine

