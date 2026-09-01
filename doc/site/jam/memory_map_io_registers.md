
# Memory map & I/O Registers

(TODO before release: clean up here and add labels from jam.inc)

## Memory map

- $0000-$0001: graphics port (not implemented yet)
- $0002-$0003: reserved for later use
- $0004-$0007: zeropage RAM reserved for kernel
    - $04/5: temporary vector used for PRINT and CP/M fs
    - $06: save processor status for PRINT
    - $07: save accumulator for PRINT
- $0008-$00FF: zeropage RAM for generic use
    - $0008-$000F: zeropage RAM used by WozMon
    - $00E3-$00FF: zeropage RAM used by TIM
    - $00F6-$00FF: zeropage RAM used by System Monitor
- $0100-$01FF: processor stack
- $0200-$03FF: RAM reserved for kernel (e.g. CP/M fs, VT100)
- $0400-$CFFF: RAM for generic use
- $D000-$DDFF: I/O which can be provided by external boards
- $D000-$D3FF: scratch RAM that can by exchanged with $0000-$03FF
    by writing to $DF03 (not accessable directly from 65C02)
- $DE00-$DEFF: I/O space reserved for future use provided by main RP2040 board
- $DF00-$DFFF: I/O provided by main RP2040 board
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


### Overview

- $0000-$0001: VGA
- $0002-$0003: (reserved)
- $D000-$D2FF: (undefined)
- $D300-$D3FF: 32x32 LED Framebuffer (suggested)
- $D400-$D4FF: Sound: SID clone(s), mod player (suggested)
- $D500-$DDFF: (undefined)
- $DE00-$DEFF: (reserved)
- $DF00-$DF7F: (internal: see below)
- $DF80-$DFFF: (scratch RAM for internal drive access)

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
    bits 4,5 define mode:
    - $10 --> store
    - $20 --> read
    - $30 --> swap
    bit 7 (R): action ongoing
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
    - bit 1: enable flow control (removed due to rework)
    - bit 2-3: select auto UTF-8 conversion
        - 00: off
        - 01: standard Sorbus conversion (not fully defined yet)
        - 10: reserved
        - 11: reserved
    - bit 6: overflow in output uart fifo occurred (will be cleared on read)
    - bit 7: overflow in input uart fifo occurred (will be cleared on read)
- $DF0C: (R) UART in queue read
- $DF0D: (R) serial in queue size (up to 250, 255: error)
- $DF0E: (W) serial out queue write
- $DF0F: (R) serial out queue size (up to 127, >127: error)

### Timers ($DF10-$DF1F)

- 8 different 16 bit timers are available allowing for an own timer for
  every combination, see table below
- writing to lowbyte also stops timer
- writing to highbyte also starts timer
- reading from either low- or highbyte shows if timer triggered interrupt
  ($80) and also acknoledges interrupt

| Address | Repeating | Interrupt | Time Source |
| ------- | --------- | --------- | ----------- |
| $DF10/1 | repeat    | IRQ       | cycles      |
| $DF12/3 | oneshot   | IRQ       | cycles      |
| $DF14/5 | repeat    | NMI       | cycles      |
| $DF16/7 | oneshot   | NMI       | cycles      |
| $DF18/9 | repeat    | IRQ       | 10ths of ms |
| $DF1A/B | oneshot   | IRQ       | 10ths of ms |
| $DF1C/D | repeat    | NMI       | 10ths of ms |
| $DF1E/F | oneshot   | NMI       | 10ths of ms |

Note: when entering [meta mode](meta_mode.md), the ms based timers will
be cancelled. The will be restarted from their initial value when meta
mode is exited.

### Watchdog ($DF20-$DF23)

Watchdog will be triggered after the specified number of cycles.
However, there is no easy way to reset the watchdog counter. It needs
to be set again to full value for next "loop".

- counter is 24 bit
- base address: $DF20
- write to base address + 0: turn off
- write to base address + 1: set low counter
- write to base address + 2: set mid counter
- write to base address + 3: set high counter, starts watchdog
- read on $DF20 shows watchdog active (bit7: watchdog running)
- read on $DF20-$DF23 also re-arms the watchdog
- triggered watchdog is handled similar to trap ($DF01)


### Cyclecount ($DF24-$DF27)

- read only 32 bit register
- writing to any address of the range copies the cycle counter to RAM
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

System provides 36864 blocks of 128 bytes = 4.5MB Data stored in flash @
0x10400000 (12MB, ~<6MB payload with wear leveling)

- base address: $DF70
- base address + $0: LBA low
- base address + $1: LBA high
- base address + $2: DMA memory address low
- base address + $3: DMA memory address high
- base address + $4: (S) read sector (strobe, adjusts DMA memory and LBA)
- base address + $5: (S) write sector (strobe, also adjusts)
- base address + $6: (unused, see variables used by kernel)
- base address + $7: (S) flash discard

Valid values are:

- DMA memory start address:
    - $0004-$CF80
    - $DF80-$FF80 (will always use RAM under ROM)
- LBA:
    - $0000-$7FFF (used by CP/M-fs)
    - $8000-$8FFF (reserved for future use, e.g. ROM-able Forth)

Each transfer stops CPU until transfer is completed. Reading from strobe
registers return result of last access. (Bit 7 set indicates error.)
Memory address will be increased by $0080, LBA by $0001 after successful
read or write to prepare for next sector transfer.


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
