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
- $0004-$000F: zeropage RAM reserved for kernel
- $0010-$00FF: zeropage RAM for generic use
- $0100-$01FF: stack
- $0200-$03FF: RAM reserved for kernel (e.g. CP/M fs)
- $0400-$CFFF: RAM for generic use
- $D000-$DEFF: I/O provided by external boards
- $DF00-$DFFF: I/O provided by main RP2040 board
- $E000-$FFFF: bank 0 (RAM, used to load CP/M 65)
- $E000-$FFFF: bank 1 (ROM, custom firmware)
- $FF00-$FFFF: bankswitching code, BRK handler, base routines
               can be copied to RAM using code at $0100 after loading
               a bootblock


Internal I/O ($DF00-$DFFF)
==========================
The following memory areas are space reserved for that functional group.


Miscellaneous ($DF00-$DF0F)
---------------------------
- $DF00: bank select register for $E000-$FFFF
         ROM starts at bank 1 (default), bank 0 is RAM
         more banks can be added, if bank set > max_bank, bank is set to default
- $DF01: (R) Sorbus Native ID -> contains version information, read until $00
         starts with ASCII id "SBC23", followed by a $01 and a revision
- $DF01: (S) trap: stop CPU and jump into debugging console
- $DF02: (R) random value
- $DF03-$DF0A: reserved for future use
- $DF0B: UART config: bit 0=enable crlf conversion
- $DF0C: (R) UART in queue read
- $DF0D: (R) serial in queue size (up to 240, 255: error)
- $DF0E: (W) serial out queue write
- $DF0F: (R) serial out queue size (up to 127, >127: error)


Timer ($DF10-$DF1F)
-------------------
- two 16 bit timers triggering either IRQ or NMI
- base address IRQ timer: $DF10
- base address NMI timer: $DF14
- base address + 0 = set low counter for repeating timer, stops timer
- base address + 1 = set high counter for repeating timer, starts timer
- base address + 2 = set low counter for single shot timer, stops timer
- base address + 3 = set high counter for single shot timer, starts timer
- reading any register return $80 if timer was triggered, $00 otherwise
- reading clears flag and also resets IRQ or NMI line back to high
- IMPORTANT: this might change, if 16-bit counters are not sufficiant


Watchdog ($DF20-$DF23)
----------------------
- counter is 24 bit
- base address: $DF20
- base address + 0: turn off
- base address + 1: set low counter, write resets watchdog when running
- base address + 2: set mid counter, write resets watchdog when running
- base address + 3: set high counter, stars watchdog, reset when running
- read on any address shows watchdog active
- triggered watchdog handled similar to trap ($DF01)
- todo(?): can be triggered by number of nmis or irqs


Internal Drive ($DF70-$DF77)
----------------------------
System provides 32768 blocks of 128 bytes = 4MB
Data stored in flash @ 0x10400000 (12MB, ~<6MB payload with wear leveling)
LBA: block index, allowed $0000-$7FFF (4MB for OS)
Additional blocks not used by OS
DMA memory: allowed $0004-$CF80, $DF80-$FF80
- base address: $DF70
- base address + $0: LBA low
- base address + $1: LBA high
- base address + $2: DMA memory low
- base address + $3: DMA memory high
- base address + $4: (S) read sector (strobe, adjusts DMA memory and LBA)
- base address + $5: (S) write sector (strobe)
- base address + $6: (unused)
- base address + $7: (S) flash discard
Each transfer stops CPU until transfer is completed


RAM Vectors ($DF78-$DF7F)
-------------------------
These vectors are RAM to support installing own handlers for interrupts
- $DF78/$DF79 user BRK routine (if BRK operand is $00 or out of scope)
- $DF7A/$DF7B NMI ($FFFA/B point to jmp ($DF7A))
- $DF7C/$DF7D user IRQ routine (for handling non-BRK)
- $DF7E/$DF7F IRQ ($FFFE/F point to jmp ($DF7E))
Note: TIM overwrites vectors for own debugging purposes, WozMon doesn't.

Interrupt Handling
------------------
1) $FFFE/F is triggered by IRQ-line or BRK
2) jmp ($DF7E) -> default setup to handler in $FF00 area
3) handler checks if trigger was IRQ or BRK
4) if IRQ -> jmp ($DF7C)
5) if BRK get operand after BRK
6) if operand is known, perform kernel action
7) if operand is out of scope -> jmp ($DF78)
Note: as this handling is rather complex it takes about 100 cycles to
run a software interrupt to call a function. This is the trade-in for
convenience. Also, all registers get saved/restored during a software
interrupt.

Kernel Interrupts
-----------------
- $00: jmp ($DF78)
- $01: chrinuc: wait for key and return it uppercase
- $02: chrcfg: set UART configuration parameters
- $03: prhex8: output accumulator as 2 digit hex value
- $04: prhex16: output X and accumulator as 4 digit hex value
- $05: CP/M-fs set filename: convert filename (pointer in X/A), Y=userid
- $06: CP/M-fs load: load file to address in ($030c/d)
- $07: CP/M-fs save: save file from address in ($030c/d) to ($030e/f)
- $08: CP/M-fs erase: delete file
- $09: CP/M-fs directory: load directory to address in ($030c/d) or screen

Scratchpad RAM ($DF80-$DFFF)
----------------------------
128 bytes of RAM intended to be used to store a sector from internal drive,
e.g. directory data.


Unused addresses in $DF00-$DF7F behave like RAM, except that they can't be
used with internal drives DMA.


Suggested External I/O Addresses
================================
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


Notes On Implementation in RP2040
=================================

Multicore Architecture
----------------------
Core 0 runs the console and handles user interaction. Core 1 drives the
bus for the CPU implementing the system. To have a rather efficient
(fast as in ~1MHz) system, core 1 really has to come up with some tricks.
So, every "event" aspect, such as timers, watchdog and other things will
be run by an event queue. This means on every clock cycle there is a
check if something was scheduled for this specific clock cycle. Again
due to performance, only one event will happen during that clock cycle.
If two events are scheduled for the same clock cycle, the second one
scheduled will be delay by one clock cycle (and again until there is a
free slot). The size of the queue is 32 event. This should be sufficiant,
as there are not much things that could add to the event queue, and in
most cases, a new event from the same source replaces the old one.

However: again due to performance the queue is not thread safe! So only
core 1 is allowed to interact with it.

Core 0 on the other is allowed to do two things on the hardware side:
- pull the RDY line low (to stop the CPU) and back high (to let the CPU
  continue)
- pull the RESET line to low (but not back to high again), this may be
  only done if the RDY line is high, or immediately pulled high after
  the reset line was pulled low (immediately = in the next code line!)

If core 1 should want to use the RDY line for some reason, this should
be implemented using "queue_uart_write" with characters > 0xFF.
