
; $FF00: read a character from UART in A, returns C=1 when no queue is empty
.global  CHRIN

; $FF03: write a character from A to UART
.global  CHROUT

; $FF06: set UART config: C=1 sets bits, C=0 clears bits in A
; bit 0: CRLF conversion (disabling also disables Meta-Key for RP2040 console)
.global  CHRCFG

; $FF09: print a string
; usage:
; jsr PRINT
; .byte "text", 0
; this routine saves all CPU registers, including P
.global  PRINT

