
; jumptables in banks > 1 for bank 1 to call

; bank 2: helpers
B2UNH816       = $E000  ; must be first
B2GENSINE      = $E003  ; jsr
B2BROWSER      = $E006  ; jmp
B2RUN6502ASM   = $E009  ; jmp

; bank 3: BASIC (only jmps)
B3BASIC        = $E000  ; must be first
B3AUTORUNBAS   = $E003

; bank 4: FORTH (only jmps)
B4FORTH        = $E000  ; must be first
