
# Kernel Routines

(TODO before release: clean up here and add labels from jam_bios.inc)

## Jumptable

### CHRIN ($FF00): read char

```txt
; $FF00: read a character from UART in A, returns C=1 when queue is empty
CHRIN    := $FF00
```
Read a character from the UART.

returns:

- carry: clear = A contains valid data, set = no data available
- A: read char

### CHROUT ($FF03): write char

```txt
; $FF03: write a character from A to UART
CHROUT   := $FF03
```

Write the content of A to the UART.

returns:

- nothing

### PRINT ($FF06): print

```txt
; $FF06: print a string
; usage:
; jsr PRINT
; .byte "text", 0
; this routine saves all CPU registers, including P, so it can be used for
; debugging messages
PRINT    := $FF06
; PRIMM is a common used synonym for PRINT here
PRIMM    := PRINT
```

Writes a string followed by the JSR, terminated with a $00 byte to the
terminal. Usage is like this (code for ca65):

```asm65c02
   jsr $FF06
   .byte "hello world",10
   .byte 0
   ; continue code here
```

The best part about this routine is that _everything_ gets saved. Even
the processor status. This means that this routine can be used for
debugging prints.


## Software Interrupts

All interrupts are named, names defined in `src/65c02/jam/jam_bios.inc`.
This include also contains a wrapper `INT` which should be used instead
of the `BRK` opcode for kernel interrupts. Possible arguments are:

### MONITOR ($00)

#### jump to System Monitor

```txt
; jump into system monitor
; this is also configured as the default for INTUSER
.define MONITOR   $00
```

This stops the current program and jumps into the
[system monitor](sysmon.md)
The program can then be continued by entering the "G" command.

### CHRINUC ($01)

#### wait for key and return it uppercase

```txt
; wait for input from CHKIN and convert it to uppercase
; in:
; -
; out:
; A: character read
.define CHRINUC   $01
```

This is a convenience function intended for menus and alike. It will
wait for a key press, converts it to uppercase and returns it as A.

### CHRCFG ($02)

#### set UART configuration parameters

```txt
; configure CHRIN/CHROUT behaviour
; in:
; A: bits to set
; X: bits to clear (done first)
; out:
; A: previous state of config
.define CHRCFG    $02
```


### PRHEX8 ($03)
#### output accumulator as 2 digit hex value

```txt
; print out an 8 bit hex value
; in:
; A: value
; out:
; -
.define PRHEX8    $03
```

### PRHEX16 ($04)
#### output X and accumulator as 4 digit hex value

```txt
; print out an 8 bit hex value
; in:
; X: value hibyte
; A: value lobyte
; out:
; -
.define PRHEX16   $04
```

### CPMNAME ($05)
#### CP/M-fs set filename: convert filename (pointer in X/A), Y=userid

```txt
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
```

Like on the commodore, the file handling is separated into a few routines.
This routine sets the file name. This is required for load save and delete.

### CPMLOAD ($06)
#### CP/M-fs load: load file to address in ($030c/d)

```txt
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
```

The load and save are done using DMA transfers. Those can only copy a
full sector of 128 bytes per DMA. So, if the last sector of a save is
only partially used, still the whole 128 bytes are written to storage,
even though the directory entry contains the correct size of the file.
The load routine does the same: it loads a full 128 bytes sector
overwriting memory with an usused part of the file. The end address of
the file in address ($030e/f) does state the correct end, but up to 127
bytes after that address might be corrupted!

### CPMSAVE ($07)
#### CP/M-fs save: save file from address in ($030c/d) to ($030e/f)

```txt
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
```

This works similar the CPMLOAD, except that the file is saved and not
loaded.

### CPMERASE ($08)
#### CP/M-fs erase: delete file

```txt
; erase a file on internal drive CP/M filesystem
; in:
; -
; $0300-$030b: filename as prepared by CPMNAME
.define CPMERASE  $08
```

This works similar the CPMLOAD, except that the file is erased and not
loaded.

### CPMDIR ($09)
#### CP/M-fs directory: load directory to address in ($030c/d) or console ($030d=$00)

```txt
; load or display internal drive CP/M filesystem directory
; in:
; Y: user partition ($00-$0f)
; $030c: load address lo
; $030d: load address hi
; if load address hi = $00 display on screen using CHROUT
.define CPMDIR    $09
```

The following rountine will print out the directory on the screen:

```asm65c02
   ldy #$0a    ; user number
   stz $030d
   brk #$09
```

When loading to memory data records are 16 bytes of size:

- $00: user number
- $01-$08: filename, padded with spaces
- $09-$0B: extension, padded with spaces
- $0c: $00
- $0d: number of bytes in final sector ($00 = full sector)
- $0e/$0f: number of sectors (lo/hi)


### VT100 ($0A)
#### VT100: several screen functions: Y=specify function (see below)

```txt
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

.define VT100_CPOS_SOL $FE ; combined functions CPOS_*: goto start of line
.define VT100_SCRN_SIZ $FF ; combined functions CPOS_*: get size of screen
.define VT100_SCRN_CL0 $FD ; combined functions SCRN_CLR: and going to top left
```

VT100 calls are identified by the function number passed via the Y
register. Some functions require / return parameters handed over via the
A and X registers.

- $00: set cursor pos (in: X=col, A=row, 1 based)
- $01: set scroll area (in: A=start, X=end)
- $02: set text attributes (colors) (30..37 bgcol, 40..47 fgcol, DECIMAL)
- $03: get cursor pos (out: X=col, A=row, 1 based)
- $04: clear screen
- $05: clear to end of line
- $06: reset scroll area
- $07: scroll down
- $08: scroll up
- $09: save cursor pos
- $0A: restore cursor pos
- $FD: combined functions: clear screen and go to top left
- $FE: combined functions: go to start of current line
- $FF: combined functions: get size of screen

### COPYBIOS ($0B)
#### copy BIOS ($FF00-$FFFF) from ROM to RAM

```txt
; copy the BIOS ($FF00-$FFFF) from ROM to RAM, so $E000-$FFFF can be banked
; to RAM without losing the functionality of the kernel
; also switches to RAM bank on return
.define COPYBIOS  $0B
```

### LINEINPUT ($0C)
#### input a text line from console (pointer in X/A, Y: size of input ($00-$7F), add $80 for only upper case)

```txt
; input text using a line
; A/X: vector to buffer, if first byte is not $00, edit the content
; Y bit 7=0: bits 0-6: maximum length of input
; Y bit 7=1: bits 0-6 get shifted to 1-7 and are ZP address of config block
; (config block not implemented yet)
; returns:
; Z=0: Ctrl-C, CRSR-up/down, etc.
; Z=1: Return
; Y: input length
; unused end of buffer will be padded with $00 bytes
.define LINEINPUT $0C
```

### GENSINE ($0D)
#### fill a page of RAM with sine data

```txt
; generate a sine table
; A=bits 0-4: size ($01-$10)
;   bit    5: write decimal parts to following page
;   bits 6-7: variant (offset in 90 degrees)
; X=page for table
.define GENSINE   $0D
```

input data:

- A: bits 7,6 offset, 5 fractions, 4-0 amplitude ($01-$10)
- X: page

Example:

```asm65c02
   lda #$10
   ldx #$cf
   int GENSINE
```

After that run the system monitor an examine the $CFxx page:

```txt
>mcf00 d000
 :CF00  FF FF FF FF FF FF FE FE  FD FD FC FB FB FA F9 F8  ................
 :CF10  F7 F6 F5 F4 F2 F1 F0 EE  ED EB EA E8 E6 E4 E2 E0  ................
 :CF20  DE DC DA D8 D6 D4 D1 CF  CC CA C7 C4 C2 BF BC B9  ................
 :CF30  B6 B3 B0 AD A9 A6 A3 9F  9C 98 95 91 8D 89 85 81  ................
 :CF40  7E 7A 76 72 6E 6A 67 63  60 5C 59 56 52 4F 4C 49  ~zvrnjgc`\YVROLI
 :CF50  46 43 40 3D 3B 38 35 33  30 2E 2B 29 27 25 23 21  FC@=;8530.+)'%#!
 :CF60  1F 1D 1B 19 17 15 14 12  11 0F 0E 0D 0B 0A 09 08  ................
 :CF70  07 06 05 04 04 03 02 02  01 01 00 00 00 00 00 00  ................
 :CF80  00 00 00 00 00 00 01 01  02 02 03 04 04 05 06 07  ................
 :CF90  08 09 0A 0B 0D 0E 0F 11  12 14 15 17 19 1B 1D 1F  ................
 :CFA0  21 23 25 27 29 2B 2E 30  33 35 38 3B 3D 40 43 46  !#%')+.0358;=@CF
 :CFB0  49 4C 4F 52 56 59 5C 60  63 67 6A 6E 72 76 7A 7E  ILORVY\`cgjnrvz~
 :CFC0  81 85 89 8D 91 95 98 9C  9F A3 A6 A9 AD B0 B3 B6  ................
 :CFD0  B9 BC BF C2 C4 C7 CA CC  CF D1 D4 D6 D8 DA DC DE  ................
 :CFE0  E0 E2 E4 E6 E8 EA EB ED  EE F0 F1 F2 F4 F5 F6 F7  ................
 :CFF0  F8 F9 FA FB FB FC FD FD  FE FE FF FF FF FF FF FF  ................
```

See, there now is a nice cosine. The offset moves it by $40 bytes. The
amplitude can be made smaller, so the value spectrum is not $00-$FF
anymore.

### ASCEND256 ($0E)
#### fill page with an ascending order of bytes

```txt
; fill a page with an ascending number of 256 values
; in:
; X=page for table
; out:
; -
.define ASCEND256 $0E
```

so low byte matches data byte (X: page)

This allows for some interesting tricks:

```asm65c02
   ldx #$cf
   int ASCEND256
   ; ... some more code ...
   and $cf00,x ; this now performs an AND with the value of the X register
```
Of course, this can be done with all other operations like add, or, eor,
etc. It makes the CPU a bit more versatile, and just costs a page of RAM
and 4(!) bytes of code.

### FB32X32 ($0F)
#### setup FB32x32 LED Matrix (framebuffer start: X/A. Y=$01 clear)

```txt
; set 32x32 pixels framebuffer registers
; Y=$00 reset LED registers to framebuffer @ X/A
; Y=$01 as $00, but also clear framebuffer with $00
.define FB32X32   $0F
```

### SWEET16 ($10)
#### enter SWEET16 interpreter

```txt
; execute SWEET16 code following the BRK
; registers are stored at $E0-$FF
; no parameters modified
.define SWEET16   $10
```

Typically [SWEET16](sweet16.md) is entered using a `JSR`. Here it's
interrupt $10 = 16, so `BRK #16`.

### PRDEC8 ($11)
#### output accumulator as 3 digit decimal value with leading zeros

```txt
; print out an 8 bit decimal value with leading 0s
; in:
; A: value
; out:
; -
.define PRDEC8    $11
```

This behaves roughly like PRHEX8 ($03), except that the value is now
printed as a decimal number. However, it will be always printed as three
digits with leading zeros, so a "7" will always be a "007".

### PRDEC16 ($12)
#### output X and accumulator as 5 digit decimal value with leading zeros

```txt
; print out a 16 bit decimal value with leading 0s
; in:
; X: value hibyte
; A: value lobyte
; out:
; -
.define PRDEC16   $12
```

This behaves roughly like PRHEX16 ($04), except that the value is now
printed as a decimal number. However, it will be always printed as five
digits with leading zeros, so a "7" will always be a "00007".

### USER-INT ($80)
#### jmp ($DF78)

```txt
; will jump using vector UVBRK ($DF78/$DF79)
; will also be triggered by any undefined BRK
; (for own extensions, interrupts >=$80 are reserved)
; default configuration is system monitor
; in/out: depends on user implementation
.define INTUSER   $80
```

For an own interrupt handler invoked via $DF78/9, it is recommended to
use interrupt arguments starting with $80, as those won't be used by the
kernel. Still, _every_ interrupt not handled by the kernel will be
passed on.

Also note that registers are not stored on the stack, but in memory.
So, running an interrupt within an interrupt will corrupt registers.
(It might work with saving $DF2C-$DF2F, but this was not tested yet.)

The default setup for the $DF78 vector is entering the system monitor.
