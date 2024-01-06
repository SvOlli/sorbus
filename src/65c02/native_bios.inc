
; $FF00: read a character from UART in A, returns C=1 when no queue is empty
.global  CHRIN

; $FF03: write a character from A to UART
.global  CHROUT

; $FF06: print a string
; usage:
; jsr PRINT
; .byte "text", 0
; this routine saves all CPU registers, including P
.global  PRINT

; non-API addresses required for using BIOS <-> kernel
.global  BIOS
.global  IRQCHECK
.global  brkjump        ; kernel only, doesn't matter in other banks
.global  reset          ; kernel only, doesn't matter in other banks

.macro int arg
   .byte $00,arg        ; BRK with operand
.endmacro

; will jump using vector UVBRK ($DF78/$DF79)
; default configuration is trap
; in/out: depends on user implementation
.define BRKUSER   $00

; wait for input from CHKIN and convert it to uppercase
; in:
; -
; out:
; A: character read
.define CHRINUC   $01

; configure CHRIN/CHROUT behaviour
; in:
; A: bits to set
; X: bits to clear (done first)
; out:
; A: previous state of config
.define CHRCFG    $02

; print out an 8 bit hex value
; in:
; A: value
; out
; -
.define PRHEX8    $03

; print out an 8 bit hex value
; in:
; X: value hibyte
; A: value lobyte
; out
; -
.define PRHEX16   $04

; convert a filename from "NAME.EXT" to format required for load/save/delete
; in:
; A: pointer to filename lo
; X: pointer to filename hi
; Y: user partition ($00-$0f)
; out:
; -
; $0300-$030b: filename in format: St F0 F1 F2 F3 F4 F5 F6 F7 E0 E1 E2
.define CPMNAME   $05

; load a file from internal drive CP/M filesystem
; in:
; -
; $0300-$030b: filename as prepared by CPMNAME
; $030c: load address lo
; $030d: load address hi
; out:
; C=1 on error
; NOTE: can only load full sectors (128 bytes)
.define CPMLOAD   $06

; save a file to internal drive CP/M filesystem
; in:
; -
; $0300-$030b: filename as prepared by CPMNAME
; $030c: start address lo
; $030d: start address hi
; $030e: end address + 1 lo
; $030f: end address + 1 hi
; out:
; C=1 on error
; NOTE: can only save full sectors (128 bytes)
.define CPMSAVE   $07

; erase a file on internal drive CP/M filesystem
; in:
; -
; $0300-$030b: filename as prepared by CPMNAME
.define CPMERASE  $08

; load or display internal drive CP/M filesystem directory
; in:
; Y: user partition ($00-$0f)
; $030c: load address lo
; $030d: load address hi
; if load address hi = $00 display on screen using CHROUT
.define CPMDIR    $09

; render VT100 escape sequences
; Y: sequence id
;   $00: set cursor pos (in: X=col, A=row, 0 based)
;   $01: set scroll area (in A=start, X=end)
;   $02: set text attributes (colors) (30..37 bgcol, 40..47 fgcol, DECIMAL)
;   $03: get cursor pos (out: X=col, A=row, 1 based)
;   $04: clear screen
;   $05: clear to end of line
;   $06: reset scroll area
;   $07: scroll down
;   $08: scroll up
;   $09: save cursor pos
;   $0A: restore cursor pos
.define VT100     $0A
