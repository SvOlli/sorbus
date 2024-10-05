;
; Copyright 2023-2024 Sven Oliver ("SvOlli") Moll
; Distributed unter the terms: GPL-3.0-or-later
;

; $FF00: read a character from UART in A, returns C=1 when no queue is empty
.define CHRIN  $FF00

; $FF03: write a character from A to UART
.define CHROUT $FF03

; $FF06: print a string
; usage:
; jsr PRINT
; .byte "text", 0
; this routine saves all CPU registers, including P, so it can be used for
; debugging messages
.define PRINT  $FF06

; non-API addresses required for BIOS <-> kernel calls
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
.define INTUSER   $00

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

; used by CPM filesystem routines
.define CPM_FNAME $0300
.define CPM_SADDR CPM_FNAME+$0c
.define CPM_EADDR CPM_FNAME+$0e
; full memory block starts at $0300-$035a
; (more if a file is loaded that is larger than 64k)
; during save, the memory $0200-$02ff is used for available blocks map

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
; Y: sequence id (see below)
.define VT100     $0A

.define VT100_CPOS_SET $00 ; set cursor pos (in: X=col, A=row, 1 based)
.define VT100_CPOS_GET $03 ; get cursor pos (out: X=col, A=row, 1 based)
.define VT100_CPOS_SAV $09 ; save cursor pos
.define VT100_CPOS_RST $0A ; restore cursor pos

.define VT100_SCRL_LIN $01 ; set scroll area in lines (in A=start, X=end)
.define VT100_SCRL_RES $06 ; reset scroll area
.define VT100_SCRL_DWN $07 ; scroll down
.define VT100_SCRL_UP  $08 ; scroll up

.define VT100_COLORS   $02 ; set text attributes (colors)
                           ; (30..37 bgcol, 40..47 fgcol, DECIMAL)
.define VT100_SCRN_CLR $04 ; clear screen
.define VT100_EOLN_CLR $05 ; clear to end of line

; copy the BIOS ($FF00-$FFFF) from ROM to RAM, so $E000-$FFFF can be banked
; to RAM without losing the functionality of the kernel
.define COPYBIOS  $0B

; input text using a line
; A/X: vector to buffer, if first byte is not $00, edit the content
; Y: bits 0-6: maximum length of input, bit 7: convert to uppercase
; returns:
; C=1: Ctrl-C
; C=0: Return
; Y: input length
; unused end of buffer will be padded with $00 bytes
.define LINEINPUT $0C

.define GENSINE   $0D

