
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
