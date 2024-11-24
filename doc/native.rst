Sorbus Native
=============

The Sorbus Native (required: a better name) is an implementation of a
complete system that does not resemble any already created computer. It
is intended to grow into a well defined platform capable of running
demos.

The flash of the RP2040 contains the Kernel ROM and the flash filesystem
image in RAW flash. Those can (and have to) be loaded independently from
the core in flash, using the following commands:

-  picotool load -o 0x103FA000 -t bin <kernel>
-  picotool load -o 0x10400000 -t bin <filesystem>

(This is a subject to change, because it's now possible to create a UF2
file with everything firmware.)

Available Boot Options
----------------------

-  WozMon
-  TIM (MOS 6530-004)
-  OSI BASIC V1.0 REV 3.2

Those are ports of the original software which for example make use of
the additional opcodes of the 65C02.

-  Filebrowser

This is a simple file loader. It loads files with the extension “.SX4”
from the user-partition 10.

-  CP/M 65

This can be loaded from bootblock 0. It is a reimplementation of the
original 8080 code for the 6502.

There are still three other bootblocks available for other use.
Bootblocks are always 8k in size, they will be loaded at $E000 at the
RAM bank 0 “under” the kernel area. The first three bytes should include
a JMP instruction followed by the sequence of the ASCII characters
"SBC23" (without the quotes). Upon startup, a routine will be installed
at $0100, that copies the last page of memory ($FF00-$FFFF) from kernel
to RAM. This will enable the loaded code to utilize the software
interrupt handler and other features of the kernel. This part is called
the BIOS.

Using an NMOS 6502
------------------

The firmware of this core does not support an NMOS 6502, and shows an
appropriate message upon startup. However, is it possible to drop into
WozMon. This can be used for some rudimentary work. It is also possible
to run Instant Assembler within WozMon using the tool ``wozcat``.
Furthermore, it is planned to have a more sophisticated monitor
included in the future.


CP/M 65 Usage
~~~~~~~~~~~~~

Upon startup, ``CCP.SYS``, the command interpreter, is loaded. It knows
the built-in commands ``DIR``, ``ERA``, ``REN``, ``TYPE``, ``USER`` and
``FREE``.

`Repository of CP/M 65 <https://github.com/davidgiven/cpm65>`__

OSI BASIC V1.0 REV 3.2 Usage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This port has some significant changes compared to the original OSI
BASIC.

-  The zeropage starts at $10, not $00, so everything is moved there.
-  user jumpcode is still at $0a
-  NULL command removed, token replaced with SYS that behaves similar
   to Commodore BASIC
-  $0d (13), $0e (14), $0f (15) are used as A,X,Y when calling SYS
-  As the full code has been disassembled, modified and reassembled,
   all addresses of the ROM functions are not correct.
-  The monitor has been replaced with routines for LOAD, SAVE, DIR and
   the BIOS. LOAD and SAVE take a filename for a parameter. “.BAS” is
   appended automatically. Those files utilize the user-partition 11.
   SAVE erases a previous file with the same name before writing.
-  BASIC starts at $0400, as the area $0200-$03ff is used by the Sorbus
   kernel for accessing the internal drive. ($0200-$02ff is only used
   for saving.)
-  The LOAD and SAVE work totally different as stated in the manuals.
-  LOAD"$" is added to display the directory on screen.
-  No need to enter size of memory size (compiled in)
-  terminal width and auto-newline has totally been removed and should
   be now handled by the terminal software used
-  Opcode order is kept the same, so most original OSI BASIC code
   should work on the Sorbus variant as well
-  code for RND(0) has been changed, so sequence will not be the same,
   actually being random

SX4 File Format For Executables
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SX4 stands for Sorbus eXecutable $0400, so it's just a binary blob
loaded to $0400 at memory and started at the load address, after the
bank has been switched to $00 (RAM @ $E000) with the BIOS copied to RAM.

Memory map
----------

-  $0000-$0001: graphics port
-  $0002-$0003: for later use
-  $0004-$0007: zeropage RAM reserved for kernel

   -  $04/5: temporary vector used for PRINT and CP/M fs
   -  $06: save processor status for PRINT
   -  $07: save accumulator for PRINT

-  $0008-$00FF: zeropage RAM for generic use
   -  $0008-$000F: zeropage RAM used by WozMon
   -  $00E3-$00FF: zeropage RAM used by TIM
-  $0100-$01FF: stack
-  $0200-$03FF: RAM reserved for kernel (e.g. CP/M fs, VT100)
-  $0400-$CFFF: RAM for generic use
-  $D000-$DEFF: I/O provided by external boards
-  $D000-$D3FF: scratch RAM that can by exchanged with $0000-$03FF
   by writing to $DF03 (not accessable directly from 65C02)
-  $DE00-$DEFF: internal temporary buffer (not accessable from 65C02)
-  $DF00-$DFFF: I/O provided by main RP2040 board
-  $E000-$FFFF: bank 0 (RAM, used to load CP/M 65)
-  $E000-$FFFF: bank 1 (ROM, custom firmware)
-  $E000-$FFFF: bank 2 (ROM, tools e.g. filebrowser)
-  $E000-$FFFF: bank 3 (ROM, OSI BASIC)
-  $FF00-$FFFF: bankswitching code, BRK handler, I/O routines
   -  can be copied to RAM using code at $0100 after loading a bootblock
   -  is copied to RAM before running SX4 file

Miscellaneous ($DF00-$DF0F)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

-  $DF00: (R/W) bank select register for $E000-$FFFF ROM starts at
   bank 1 (default), bank 0 is RAM; more banks can be added, if
   bank set > max_bank, bank is set to default
-  $DF01: (R) Sorbus Native ID -> contains version information, read
   until $00 starts with ASCII id "SBC23", followed by a $01 and a
   revision
-  $DF01: (S) trap: stop CPU and jump into debugging console
-  $DF02: (R) random value
-  $DF03: (W) swap out pages $00-$03: lower four bits contain banks,
   upper two bits mode: $40 -> store, $80 -> read, $c0 -> swap
-  $DF04: (R) CPU: $01=6502, $06=65C02, $12=65816, $0e=65CE02, $02=65SC02
   (bit set indicate CPU features:NMOS,CMOS,BIT (RE)SET,Z reg,16 bit)
-  $DF05-$DF0A: reserved for future use
-  $DF0A: 65CE02: userspace workaround to save Z for BRK (might change)
-  $DF0B: UART config: bit 0=enable crlf conversion
-  $DF0C: (R) UART in queue read
-  $DF0D: (R) serial in queue size (up to 240, 255: error)
-  $DF0E: (W) serial out queue write
-  $DF0F: (R) serial out queue size (up to 127, >127: error)

Timer ($DF10-$DF1F)
~~~~~~~~~~~~~~~~~~~

-  two 16 bit timers triggering either IRQ or NMI
-  base address IRQ timer: $DF10
-  base address NMI timer: $DF14
-  base address + 0 = set low counter for repeating timer, stops timer
-  base address + 1 = set high counter for repeating timer, starts timer
-  base address + 2 = set low counter for single shot timer, stops timer
-  base address + 3 = set high counter for single shot timer, starts
   timer
-  reading any register return $80 if timer was triggered, $00 otherwise
-  reading clears flag and also resets IRQ or NMI line back to high
-  IMPORTANT: this might change, if 16-bit counters are not sufficiant

Watchdog ($DF20-$DF23)
~~~~~~~~~~~~~~~~~~~~~~

-  counter is 24 bit
-  base address: $DF20
-  base address + 0: turn off
-  base address + 1: set low counter, write resets watchdog when running
-  base address + 2: set mid counter, write resets watchdog when running
-  base address + 3: set high counter, stars watchdog, reset when
   running
-  read on any address shows watchdog active
-  triggered watchdog handled similar to trap ($DF01)
-  todo(?): can be triggered by number of nmis or irqs

Cyclecount ($DF24-$DF27)
~~~~~~~~~~~~~~~~~~~~~~~~

-  read only 32 bit register
-  reading at $DF24 copies actual counter to a shadow register
-  other addresses will return timestamp as when $DF24 was accessed
-  intended to be used for measuring speed of code
-  address still subject to change

Variables Used By Kernel ($DF2C-$DF2F)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-  $DF2B: Z index register (65CE02 only, this might change)
-  $DF2C: bank
-  $DF2D: accumulator
-  $DF2E: X index register
-  $DF2F: Y index register

This are just variables used during handling an interrupt service call

Variables Used By System Monitor ($DF30-$DF3F)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This area of I/O is just used as conventional RAM to store data away from
the area used by conventional programs.

-  $DF30: saved PC lo
-  $DF31: saved PC hi
-  $DF32: bank
-  $DF33: accumulator
-  $DF34: X index register
-  $DF35: Y index register
-  $DF36: stack pointer
-  $DF37: processor status

Internal Drive ($DF70-$DF77)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

System provides 32768 blocks of 128 bytes = 4MB Data stored in flash @
0x10400000 (12MB, ~<6MB payload with wear leveling)
LBA: block index, allowed $0000-$7FFF
     (4MB for OS, additional blocks not used by OS)
DMA memory: allowed $0004-$CF80, $DF80-$FF80 for start address

-  base address: $DF70
-  base address + $0: LBA low
-  base address + $1: LBA high
-  base address + $2: DMA memory low
-  base address + $3: DMA memory high
-  base address + $4: (S) read sector (strobe, adjusts DMA memory and LBA)
-  base address + $5: (S) write sector (strobe, also adjusts)
-  base address + $6: (unused)
-  base address + $7: (S) flash discard

Each transfer stops CPU until transfer is completed. Reading from strobe
registers return result of last access. (Bit 7 set indicates error.)


RAM Vectors ($DF78-$DF7F)
~~~~~~~~~~~~~~~~~~~~~~~~~

These vectors are RAM to support installing own handlers for interrupts

-  $DF78/$DF79 user BRK routine (if BRK operand is $00 or out of scope)
-  $DF7A/$DF7B NMI ($FFFA/B point to jmp ($DF7A))
-  $DF7C/$DF7D user IRQ routine (for handling non-BRK)
-  $DF7E/$DF7F IRQ ($FFFE/F point to jmp ($DF7E))

Note: TIM overwrites vectors for own debugging purposes, WozMon doesn't.

Scratchpad RAM ($DF80-$DFFF)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

128 bytes of RAM intended to be used to store a sector from internal
drive, e.g. directory data.

Unused addresses in $DF00-$DF7F behave like RAM, except that they can't
be used with internal drive DMA.

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
~~~~~~~~~~~~~~~~~

-  $00: jmp ($DF78)
-  $01: chrinuc: wait for key and return it uppercase
-  $02: chrcfg: set UART configuration parameters
-  $03: prhex8: output accumulator as 2 digit hex value
-  $04: prhex16: output X and accumulator as 4 digit hex value
-  $05: CP/M-fs set filename: convert filename (pointer in X/A),
        Y=userid
-  $06: CP/M-fs load: load file to address in ($030c/d)
-  $07: CP/M-fs save: save file from address in ($030c/d) to ($030e/f)
-  $08: CP/M-fs erase: delete file
-  $09: CP/M-fs directory: load directory to address in ($030c/d) or
        console ($030d=$00)
-  $0A: VT100: several screen functions: Y=specify function (see below)
-  $0B: copy BIOS from ROM to RAM
-  $0C: input a text line from console (pointer in X/A,
        Y: size of input ($00-$7F), add $80 for only upper case)

For an own interrupt handler invoked via $DF78/9, it is recommended to
use interrupt arguments starting with $80, as those won't be used by the
kernel.

Also note that registers are not stored on the stack, but in memory.
This results in running an interrupt within an interrupt will corrupt
registers.


CP/M-fs Load And Save
---------------------

The load and save are done using DMA transfers. Those can only copy a
full sector of 128 bytes per DMA. So if the last sector of a save is
only partially used, still the whole 128 bytes are written to storage,
even though the directory entry contains the correct size of the file.
The load routine does the same: it loads a full 128 bytes sector
overwriting memory with an usused part of the file. The end address of
the file in address ($030e/f) does state the correct end, but up to 127
bytes after that address might be corrupted!

VT100 Calls
-----------

VT100 calls are identified by the function number passed via the Y
register. Some functions require / return parameters handed over via the
A and X registers.

-  $00: set cursor pos (in: X=col, A=row, 1 based)
-  $01: set scroll area (in: A=start, X=end)
-  $02: set text attributes (colors) (30..37 bgcol, 40..47 fgcol, DECIMAL)
-  $03: get cursor pos (out: X=col, A=row, 1 based)
-  $04: clear screen
-  $05: clear to end of line
-  $06: reset scroll area
-  $07: scroll down
-  $08: scroll up
-  $09: save cursor pos
-  $0A: restore cursor pos

Hint: to get the size of the terminal window, call $00 (set cursor pos)
with A & X set to 254, then query the cursor position with call $03. Then
the real size is reported. It might be a good idea to then set position
1, 1 (top left).

Suggested External I/O Addresses
--------------------------------

-  $D400: SID clone(s): 5-bit register select -> 8 SIDs max
-  $DA00: RIOT 6532: 1 will take up full page
-  $DB00: ACIA(s): 2-bit register select -> 64 ACIAs max
-  $DC00: VIA(s): 4-bit register select -> 16 VIAs max

Chip-Select-GAL
---------------

Using a GAL 22v10 chip

Out of 16 bit address - 8 bits 15-8 hardcoded to Dx?? - 3 bits 7-5 to
decode chip select output - 8 chip select outputs - 1 bank select output
(also used internally)

A GAL 20v8 could only decode 4 chip selects (or mayby 7, based upon
implementation)

Notes On Implementation in RP2040
---------------------------------

Multicore Architecture
~~~~~~~~~~~~~~~~~~~~~~

Core 0 runs the console and handles user interaction. Core 1 drives the
bus for the CPU implementing the system. To have a rather efficient
(fast as in ~1MHz) system, core 1 really has to come up with some
tricks. So, every “event” aspect, such as timers, watchdog and other
things will be run by an event queue. This means on every clock cycle
there is a check if something was scheduled for this specific clock
cycle. Again due to performance, only one event will happen during that
clock cycle. If two events are scheduled for the same clock cycle, the
second one scheduled will be delay by one clock cycle (and again until
there is a free slot). The size of the queue is 32 event. This should be
sufficiant, as there are not much things that could add to the event
queue, and in most cases, a new event from the same source replaces the
old one.

However: again due to performance the queue is not thread safe! So only
core 1 is allowed to interact with it.

Core 0 on the other hand is allowed to do two things on the hardware
side:

-  pull the RDY line low (to stop the CPU) and back high (to let the CPU
   continue)
-  pull the RESET line to low (but not back to high again), this may be
   only done if the RDY line is high, or immediately pulled high after
   the reset line was pulled low (immediately = in the next code line!)

If core 1 should want to use the RDY line for some reason, this should
be implemented using “queue_uart_write” with characters > 0xFF. For
performance/latency reasons it also does pull RDY low itself as well.
