
# Memory map & I/O Registers

## Memory map

- $0000-$0001: graphics port
- $0002-$0003: reserved for later use
- $0004-$0007: zeropage RAM reserved for kernel
    - $04/5: temporary vector used for PRINT and CP/M fs
    - $06: save processor status for PRINT
    - $07: save accumulator for PRINT
- $0008-$00FF: zeropage RAM for generic use
    - $0008-$000F: zeropage RAM used by WozMon
    - $00E3-$00FF: zeropage RAM used by TIM
    - $00F6-$00FF: zeropage RAM used by System Monitor
- $0100-$01FF: stack
- $0200-$03FF: RAM reserved for kernel (e.g. CP/M fs, VT100)
- $0400-$CFFF: RAM for generic use
- $D000-$DEFF: I/O provided by external boards
- $D000-$D3FF: scratch RAM that can by exchanged with $0000-$03FF
    by writing to $DF03 (not accessable directly from 65C02)
- $D300-$D3FF: I/O area suggested to be used by 32x32 framebuffer
    (write-only, as the FB32X32 hardware only handles writes on the bus)
- $DF00-$DFFF: I/O provided by main RP2040 board
    (this can change to $DE00-$DFFF, when $DFxx area is not sufficiant)
- $E000-$FFFF: bank 0 (RAM, used to load CP/M 65)
- $E000-$FFFF: bank 1 (ROM, kernal, custom firmware)
- $E000-$FFFF: bank 2 (ROM, tools e.g. filebrowser)
- $E000-$FFFF: bank 3 (ROM, OSI BASIC)
- $FF00-$FFFF: bankswitching code, BRK handler, I/O routines (BIOS)
    - same on all ROM banks
    - can be copied to RAM using code at $0100 after loading a bootblock
    - is copied to RAM before running SX4 file
    - can also copied to RAM by using BRK #$0B (see below)


## I/O Registers

### Miscellaneous ($DF00-$DF0F)

- $DF00: (R/W) bank select register for $E000-$FFFF ROM starts at
   bank 1 (default), bank 0 is RAM; more banks can be added, if
   bank set > max_bank, bank is set to default
- $DF01: (R) Sorbus JAM ID -> contains version information, read
   until $00 starts with ASCII id "SBC23", followed by a $01 and a
   revision
- $DF01: (S) trap: stop CPU and jump into debugging console
- $DF02: (R) random value
- $DF03: (W) swap out pages $00-$03: lower four bits contain banks,
    upper two bits mode:
    - $40 --> store
    - $80 --> read
    - $c0 --> swap
- $DF04: (R) CPU capabilities using bit set indicate CPU features:
    - $01: NMOS
    - $02: CMOS
    - $04: BIT (RE)SET
    - $08: Z reg
    - $10: 16 bit
    - This means for known CPUs:
    - $01: 6502
    - $06: 65C02
    - $12: 65816
    - $0e: 65CE02
    - $02: 65SC02
- $DF05-$DF0A: reserved for future use
- $DF0B: UART config:
    - bit 0: enable crlf conversion
    - bit 1: enable flow control
    - bit 2-3: select auto UTF-8 conversion
        - 00: off
        - 01: standard Sorbus conversion
        - 10: reserved
        - 11: reserved
- $DF0C: (R) UART in queue read
- $DF0D: (R) serial in queue size (up to 240, 255: error)
- $DF0E: (W) serial out queue write
- $DF0F: (R) serial out queue size (up to 127, >127: error)


### Timer ($DF10-$DF1F)

- two 16 bit timers triggering either IRQ or NMI
- base address IRQ clockcycle timer: $DF10
- base address NMI clockcycle timer: $DF14
- base address + 0 = set low counter for repeating timer, stops timer
- base address + 1 = set high counter for repeating timer, starts timer
- base address + 2 = set low counter for single shot timer, stops timer
- base address + 3 = set high counter for single shot timer, starts timer
- reading any register return $80 if timer was triggered, $00 otherwise
- reading clears flag and also resets IRQ or NMI line back to high
- base address IRQ milliseconds timer: $DF18
- base address NMI milliseconds timer: $DF1A
- base address + 0 = set lowbyte of milliseconds ($0000 turns off)
- base address + 1 = set highbyte of milliseconds
- reading any register return $80 if timer was triggered, $00 otherwise
- reading clears flag and also resets IRQ or NMI line back to high

IMPORTANT: this might change, if 16-bit counters are not sufficiant


### Watchdog ($DF20-$DF23)

- counter is 24 bit
- base address: $DF20
- base address + 0: turn off
- base address + 1: set low counter, write resets watchdog when running
- base address + 2: set mid counter, write resets watchdog when running
- base address + 3: set high counter, stars watchdog, reset when running
- read on any address shows watchdog active
- triggered watchdog handled similar to trap ($DF01)
- todo(?): can be triggered by number of nmis or irqs


### Cyclecount ($DF24-$DF27)

- read only 32 bit register
- reading at $DF24 copies actual counter to a shadow register
- other addresses will return timestamp as when $DF24 was accessed
- intended to be used for measuring speed of code
- address still subject to change


### Variables Used By Kernel ($DF2C-$DF2F)

- $DF2C: bank
- $DF2D: accumulator
- $DF2E: X index register
- $DF2F: Y index register
- $DF76: Z index register (65CE02 userland only, this might change)

This are just variables used during handling an interrupt service call.


### Variables Used By System Monitor ($DF30-$DF3F)

This area of I/O is just used as conventional RAM to store data away from
the area used by conventional programs.

- $DF30: saved PC lo
- $DF31: saved PC hi
- $DF32: bank
- $DF33: accumulator
- $DF34: X index register
- $DF35: Y index register
- $DF36: stack pointer
- $DF37: processor status


### Internal Drive ($DF70-$DF77)

System provides 32768 blocks of 128 bytes = 4MB Data stored in flash @
0x10400000 (12MB, ~<6MB payload with wear leveling)
LBA: block index, allowed $0000-$7FFF
     (4MB for OS, additional blocks not used by OS)
DMA memory: allowed $0004-$CF80, $DF80-$FF80 for start address

- base address: $DF70
- base address + $0: LBA low
- base address + $1: LBA high
- base address + $2: DMA memory low
- base address + $3: DMA memory high
- base address + $4: (S) read sector (strobe, adjusts DMA memory and LBA)
- base address + $5: (S) write sector (strobe, also adjusts)
- base address + $6: (unused, see variables used by kernel)
- base address + $7: (S) flash discard

Each transfer stops CPU until transfer is completed. Reading from strobe
registers return result of last access. (Bit 7 set indicates error.)


### RAM Vectors ($DF78-$DF7F)

These vectors are RAM to support installing own handlers for interrupts

- $DF78/$DF79 user BRK routine (if BRK operand is $00 or out of scope)
- $DF7A/$DF7B NMI ($FFFA/B point to jmp ($DF7A))
- $DF7C/$DF7D user IRQ routine (for handling non-BRK)
- $DF7E/$DF7F IRQ ($FFFE/F point to jmp ($DF7E))

Note: TIM overwrites vectors for own debugging purposes, WozMon doesn't.


#### Interrupt Handling

- $FFFE/F is triggered by IRQ-line or BRK
- jmp ($DF7E) -> default setup to handler in $FF00 area
- handler checks if trigger was IRQ or BRK
- if IRQ -> jmp ($DF7C)
- if BRK get operand after BRK
- if operand is known, perform kernel action
- if operand is out of scope -> jmp ($DF78) (default: System Monitor)

Note: as this handling is rather complex it takes about 100 cycles to
run a software interrupt to call a function. This is the trade-in for
convenience. Also, all registers get saved/restored during a software
interrupt.


### Scratchpad RAM ($DF80-$DFFF)

128 bytes of RAM intended to be used to store a sector from internal
drive, e.g. directory data.

Unused addresses in $DF00-$DF7F behave like RAM, except that they can't
be used with internal drive DMA.


## Kernel Interrupts and BIOS Routines

### BIOS Routines

- $FF00: CHRIN: read a character from UART
- $FF03: CHROUT: write a character to UART
- $FF06: PRINT: write everything after the JSR $FF06 up to the next $00
  byte using CHROUT

### Interrupts

All interrupts are named, names defined in `src/65c02/jam/jam_bios.inc`.
This include also contains a wrapper `INT` which should be used instead
of the `BRK` opcode for kernel interrupts. Possible arguments are:

- $00: jmp ($DF78)
- $01: chrinuc: wait for key and return it uppercase
- $02: chrcfg: set UART configuration parameters
- $03: prhex8: output accumulator as 2 digit hex value
- $04: prhex16: output X and accumulator as 4 digit hex value
- $05: CP/M-fs set filename: convert filename (pointer in X/A), Y=userid
- $06: CP/M-fs load: load file to address in ($030c/d)
- $07: CP/M-fs save: save file from address in ($030c/d) to ($030e/f)
- $08: CP/M-fs erase: delete file
- $09: CP/M-fs directory: load directory to address in ($030c/d) or console ($030d=$00)
- $0A: VT100: several screen functions: Y=specify function (see below)
- $0B: copy BIOS ($FF00-$FFFF) from ROM to RAM
- $0C: input a text line from console (pointer in X/A, Y: size of input ($00-$7F), add $80 for only upper case)
- $0D: fill a page of RAM with sine data (X: bits 7,6 offset, 5 fractions, 4-0 amplitude ($01-$10), A: page)
- $0E: jump to System Monitor
- $0F: setup FB32x32 LED Matrix (framebuffer start: X/A. Y=$01 clear)
- $10: prdec8: output accumulator as 3 digit decimal value with leading zeros
- $11: prdec16: output X and accumulator as 5 digit decimal value with leading zeros

For an own interrupt handler invoked via $DF78/9, it is recommended to
use interrupt arguments starting with $80, as those won't be used by the
kernel.

Also note that registers are not stored on the stack, but in memory.
This results in running an interrupt within an interrupt will corrupt
registers.


### CP/M-fs Load And Save

The load and save are done using DMA transfers. Those can only copy a
full sector of 128 bytes per DMA. So, if the last sector of a save is
only partially used, still the whole 128 bytes are written to storage,
even though the directory entry contains the correct size of the file.
The load routine does the same: it loads a full 128 bytes sector
overwriting memory with an usused part of the file. The end address of
the file in address ($030e/f) does state the correct end, but up to 127
bytes after that address might be corrupted!


### VT100 Calls

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


## Suggested External I/O Addresses

- $0000-$0001: VGA
- $D300-$D3FF: 32x32 LED Framebuffer
- $D400-$D4FF: Sound: SID clone(s), mod player
