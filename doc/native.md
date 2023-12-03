Sorbus Native
=============

The Sorbus Native (required: a better name) is an implementation of a complete
system that does not resemble any already created computer. It is intended to
grow into a well defined platform capable of running demos.

The flash of the RP2040 contains the Kernel ROM and the flash filesystem image
in RAW flash. Those can (and have to) be loaded independently from the core in
flash, using the following commands:

- picotool load -o 0x103FE000 -t bin <kernel>
- picotool load -o 0x10400000 -t bin <filesystem>

(This is a subject to change, because it's now possible to create a UF2 file
with everything firmware.)


Memory map
----------
- $0000-$0001: graphics port
- $0002-$0003: for later use
- $0004-$CFFF: RAM
- $D000-$DEFF: I/O provided by external boards
- $DF00-$DFFF: I/O provided by main RP2040 board
- $E000-$FFFF: ROM bank 0 (custom firmware)
- $E000-$FFFF: ROM bank 1 (CP/M-65 firmware containing BIOS, BDOS and CCP)

Internal I/O ($DF00-$DFFF)
--------------------------
- $DF80-$DFF9: RAM containing copy of $FF80-$FFF9 of first ROM bank
               -> switching code
- $DFFA: (R) serial in queue read
- $DFFB: (R) serial in queue size (up to 127, >127: error)
- $DFFC: (W) serial out queue write
- $DFFC: (R) serial out queue size (up to 127, >127: error)
- $DFFE: (R) random value
- $DFFF: (proposed) bank select register for $E000-$FFFF, allowing 2MB of ROM

Timer ($DF00-$DF07)
-------------------
- two timmers triggering IRQ and NMI
- base address IRQ timer: $DF00
- base address NMI timer: $DF04
- base address + 0 = set low counter for repeating timer, stops timer
- base address + 1 = set high counter for repeating timer, starts timer
- base address + 2 = set low counter for single shot timer, stops timer
- base address + 3 = set high counter for single shot timer, starts timer
- reading any register return $80 if timer was triggered, $00 otherwise
  reading clears flag; todo(?): $40 indicates timer is running

Watchdog ($DF08-$DF0B)
----------------------
- counter is 24 bit
- base address: $DF08
- base address + 0: turn off
- base address + 1: set low counter, write resets watchdog when running
- base address + 2: set mid counter, write resets watchdog when running
- base address + 3: set high counter, stars watchdog, reset when running
- read on any address shows watchdog active

- todo(?): can be triggered by number of nmis or irqs
- triggered watchdog dumps as much as useful
  - current bus state
  - RAM contents
  - configuration of internals like timer, etc.

Internal Drive ($DF70-$DF77)
----------------------------
System provides 32768 blocks of 128 bytes = 4MB
Data stored in flash @ 0x10400000 (12MB, ~<6MB payload with wear leveling)
LBA: block index, allowed $0000-$7FFF (4MB for OS)
Additional blocks not used by OS
DMA memory: allowed $0004-$CF80, $E000-$FF80
- base address: $DF70
- base address + $0: LBA low
- base address + $1: LBA high
- base address + $2: DMA memory low
- base address + $3: DMA memory high
- base address + $4: read sector (strobe, adjusts DMA memory and LBA)
- base address + $5: write sector (strobe)
- base address + $6: (unused)
- base address + $7: flash discard
Each transfer stops CPU until transfer is completed

Suggested I/O
-------------
- $D400: SID(s): 5-bit register select -> 8 SIDs max
- $DA00: RIOT: 1 will take up full page
- $DB00: ACIA(s): 2-bit register select -> 64 ACIAs max
- $DC00: VIA(s): 4-bit register select -> 16 VIAs max

Chip-Select-GA
==============
Using a GAL 22v10 chip

Out of 16 bit address
- 8 bits 15-8 hardcoded to Dx??
- 3 bits 7-5 to decode chip select output
- 8 chip select outputs
- 1 bank select output (also used internally)

A GAL 20v8 could only decode 4 chip selects
(or mayby 7, based upon implementation)
