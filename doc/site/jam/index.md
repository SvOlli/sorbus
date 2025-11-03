
# Sorbus JAM

The Sorbus JAM (short for Just Another Machine) is an implementation of a
complete system that does not resemble any already created computer. It
is intended to grow into a well defined platform capable of running
demos. (See https://en.wikipedia.org/wiki/Demoscene)

The flash of the RP2040 contains the Kernel ROM and the flash filesystem
image in RAW flash. Those can (and have to) be loaded independently from
the core in flash, using the following commands:

-  picotool load -o 0x103FA000 -t bin <kernel>
-  picotool load -o 0x10400000 -t bin <filesystem>

(This is a subject to change, because it's now possible to create a UF2
file with everything firmware, but this seems not to be fully supported.)


## Available Boot Options

When you connect to the Sorbus Computer via USB UART, you typicall see
nothing, because the output of the reset message was faster than your
connect. Press space (or any other unused key) to get the message again.
It looks something like this:

```txt
Sorbus JAM V0.6: 1-4)Bootsector, 0)Exec RAM @ $E000,
F)ilebrowser, B)ASIC, System M)onitor, T)IM, W)ozMon?
```


### WozMon & TIM (MOS 6530-004)

Those are ports of the original software which for example make use of
the additional opcodes of the 65C02. Those are described more in detail
in [monitors.md](monitors.md).


### OSI BASIC V1.0 REV 3.2

This is a port of the Microsoft BASIC 1.0 as used on the [Ohio Scientific Model
600](https://www.oldcomputers.net/osi-600.html).
It has been heavily modified. For details see also [ms_basic.md](ms_basic.md).


### System Monitor

This is a native implementation of a monitor as known from the Commodore
machines. It has got its own documentation file [sysmon.md](sysmon.md).


### Filebrowser

This is a simple file loader. It loads files with the extension “.SX4”
and others (see "Executable Formats" below) from the user-partition 10
or others.


### CP/M 65

This can be loaded from bootblock 1. It is a reimplementation of the
original 8080 operating system for the 6502.

There are still three other bootblocks available for other use.
Bootblocks are always 8k in size, they will be loaded at $E000 at the
RAM bank 0 “under” the kernel area. The first three bytes should include
a JMP instruction followed by the sequence of the ASCII characters
"SBC23" (without the quotes). Upon startup, a routine will be installed
at $0100, that copies the last page of memory ($FF00-$FFFF) from kernel
to RAM. This will enable the loaded code to utilize the software
interrupt handler and other features of the kernel. This part is called
the BIOS.

The second bootblock will be loaded when an NMOS 6502 is detected.


### NMOS 6502 Toolkit

This is a stripped down version of the System Monitor and will be loaded
from bootblock 2. It is also loaded when an NMOS 6502 CPU is detected
during startup (see below).


## Using an NMOS 6502

The firmware of this core does not support an NMOS 6502, and shows an
appropriate message upon startup. Then the kernel attempts to load
bootblock 2 (see above). If this fails it drops into WozMon. Both can be
used for some rudimentary work. Using WozMon, it is also possible to run
Instant Assembler using the host tool `wozcat`. However the NMOS 6502
toolkit should be preferred, as it is more capable than Instant Assember.


## CP/M 65 Usage

Upon startup, ``CCP.SYS``, the command interpreter, is loaded. It knows
the built-in commands ``DIR``, ``ERA``, ``REN``, ``TYPE``, ``USER`` and
``FREE``.

`Repository of CP/M 65 <https://github.com/davidgiven/cpm65>`_


## OSI BASIC V1.0 REV 3.2 Usage

This port has some significant changes compared to the original OSI
BASIC.

- The zeropage starts at $10, not $00, so everything is moved there.
- USR() jumpcode is still at $0a
- NULL command removed, token replaced with SYS that behaves similar
  to Commodore BASIC
- $0d (13), $0e (14), $0f (15) are used as A,X,Y when calling SYS
- As the full code has been disassembled, modified and reassembled,
  all addresses of the ROM functions are not correct.
- The monitor has been replaced with routines for LOAD, SAVE, DIR and
  the BIOS. LOAD and SAVE take a filename for a parameter. “.BAS” is
  appended automatically. Those files utilize the user-partition 11.
  SAVE erases a previous file with the same name before writing.
- BASIC starts at $0400, as the area $0200-$03ff is used by the Sorbus
  kernel for accessing the internal drive. ($0200-$02ff is only used
  for saving, and also "swapped out", so data is restored after SAVE.)
- The LOAD and SAVE work totally different as stated in the manuals.
- LOAD"$" is added to display the directory on screen.
- No need to enter size of memory size (compiled in)
- terminal width and auto-newline has totally been removed and should
  be now handled by the terminal software used
- Token order is kept the same, so most original OSI BASIC code should
  work on the Sorbus variant as well
- code for RND(0) has been changed, so sequence will not be the same,
  actually being random



## Executeable Formats

Executeable file formats can be loaded using the file browser and are
only distinguished by their extension. The filebrowser does whatever is
required to set up the system to run those files.


### SX4

SX4 stands for Sorbus eXecutable at $0400, so it's just a binary blob
loaded to $0400 at memory and started at the load address, after the
bank has been switched to $00 (RAM at $E000) with the BIOS copied to RAM.
This is done by the File Browser. When an SX4 is loaded otherwise, e.g.
via UART the bank configuration will be left untouched.


### SX6

SX6 stands for Sorbus eXecutable at $0600, however it differs significantly
from SX4. SX6 is a file format intended to be running executeables
written for the runtime environment of 6502asm.com a now defunct online
"fantasy system", It was an assembler combined with a runtime environment
to run executeable at $0600 with a 32x32 pixels framebuffer located at
$0200, using a C64 color map.

This implementation is using the IRQ to update the framebuffer to the
FB32X32 hardware expansion. The IRQ and NMI were never implemented in
6502asm.com, so this is fitting. However there are some changes.

Compatibility issues:

-  no other start address than $0600 can be used
-  the framebuffer is updated with constant 20 frame per second
-  $FE (random number generator) is updated only once per IRQ. If more
   random numbers per frame are required, $FF needs to be changed to
   $DF02
-  $FF (key press) is also updated only once per IRQ, use of JSR $FF00
   for getting a key instead is suggested
-  RAM at $00-$03 cannot be used, since the Sorbus has this reserved
   for I/O
-  RAM at $04-$07 should not be used as it is reserved for kernel
   functions. However when no kernel functions are used, this can be
   used.
-  RAM can be only used up to $CFFF, as $D000-$FFFF contain I/O and ROM.
-  $FC/$FD contain a 16 bit frame counter, this is actually an
   improvement
-  SEI/CLI are now implemented, turn off/on screen updates
-  all other I/O registers from the Sorbus JAM are available

The frame counter can be utilized to calculate effects according to their
frame number. Also syncing with the next frame can be implemented rather
easy:

```asm6502
   lda   $fc
loop:
   cmp   $fc
   beq   loop
```

So, this is intended with the balance of rather easy porting existing
6502asm.com software as well having some nice features to write new short
demos for the Sorbus JAM.


## Notes On Implementation in RP2040

### Multicore Architecture

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
the core handling the bus is allowed to interact with it.

The core handling the console on the other hand is allowed to do two
things on the hardware side:

-  pull the RDY line low (to stop the CPU) and back high (to let the CPU
   continue)
-  pull the RESET line to low (but not back to high again), this may be
   only done if the RDY line is high, or immediately pulled high after
   the reset line was pulled low (immediately = in the next code line!)

If core running the bus should want to use the RDY line for some reason,
this should be implemented using “queue_uart_write” with characters > 0xFF.
For performance/latency reasons it also does pull RDY low itself as well.
