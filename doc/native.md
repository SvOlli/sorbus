Sorbus Native
=============

The Sorbus '77 (or Sorbus77 for short) is an implementation of a complete
system that does not resemble any already created computer. It is intended to
grow into a well defined platform capable of running demos.

Memory map
----------

- $0000-$0001: graphics port
- $0002-$0003: for later use
- $0004-$CFFF: RAM
- $D000-$DEFF: I/O provided by external boards
- $DF00-$DFFF: I/O provided by main RP2040 board
- $E000-$FFFF: ROM (might be banked in future)

Internal I/O
------------
- $DF80-$DFF9: RAM containing copy of $FF80-$FFF9 of first ROM bank
               -> switching code
- $DFFA: (R) serial in queue read
- $DFFB: (R) serial in queue size (up to 127, >127: error)
- $DFFC: (W) serial out queue write
- $DFFC: (R) serial out queue size (up to 127, >127: error)
- $DFFE: unused
- $DFFF: (proposed) bank select register for $E000-$FFFF, allowing 2MB of ROM

Watchdog
--------
- can be enabled and disabled
- can be triggered by timer (clockcycle count)
- can be triggered by number of nmis or irqs
- renew is strobe
- triggered watchdog dumps as much as useful
  - current bus state
  - RAM contents
  - configuration of internals like timer, etc.

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
