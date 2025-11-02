
# Thanks

This project is standing on the shoulder of giants. While it was started from
scratch, a lot of other code got integrated during the development.

A lot of code was contributed from other sources, sometimes even without
the knowledge of the author(s).

This project uses software that was developed under non-free licenses.


## 65(C)02 Based Code

All of this code has been modified to some degree to fit this project, the
Sorbus Computer. The focus is always on make things work good on the hardware
and not on keeping code as original as possible.


### MOS TIM

This is the Terminal Interface Monitor (TIM) of the MOS 6530-004.
```
Copyright 1976 MOS Technology
```
License terms: unknown

* src/65c02/jam/rom/tim.s


### WozMon

This is WozMon of the Apple 1 Computer
```
Written by Steve Wozniak
Copyright 1976 Apple Computer Company
```
License terms: unknown

* src/65c02/jam/rom/woz.s


### Apple Direct Assembler/Disassembler And Monitor

```
Apple //c & Apple II
Monitor ROM Source

Copyright 1977-1983 by Apple Computer, Inc.
All Rights Reserved

S. Wozniak         1977
A. Baum            1977
John A         NOV 1978
R. Auricchio   SEP 1982
E. Beernink        1983
```
License terms: unknown

* src/65c02/jam/mon/assembler.s
* src/65c02/jam/mon/disassembler.s
* src/65c02/jam/mon/table6502.s
* src/65c02/jam/mon/table65sc02.s
* src/65c02/jam/sx4/wozmon2c.s


### Microsoft BASIC 1.0

```
This is OSI BASIC V1.0 REV 3.2
Written by Richard W. Weiland
Copyright 1977 Microsoft Co.
```
It has been used with computers manufactured by
[Ohio Scientific Inc.](https://en.wikipedia.org/wiki/Ohio_Scientific)

Reverse engineering was started by Michael Steil
Source code is based on version by Grant Searle for his simple 6502 computer.

License terms: MIT (version 1.1 which also includes OSI)

* src/65c02/jam/rom/osi_basic.s

### Instant Assembler

This is Instant 6502 Assembler for KIM-1
```
Written by Alan Cashin
```
License terms: released as public domain

* src/65c02/jam/sx4/inst-ass.s


## RP2040 Code

### Dhara - NAND flash management layer

```
Dhara - NAND flash management layer
Copyright (C) 2013 Daniel Beer <dlbeer@gmail.com>
```
License terms: BSD-2-Clause

* src/rp2040/3rdparty/dhara


### Mod Player

```
Christian Corti
Neoart Costa Rica
```
License terms: Creative Commons Attribution-Noncommercial-Share Alike 3.0

* src/rp2040/3rdparty/c-flod


### SID Emulation

```
Dag Lem: Designed and programmed complete emulation engine.
```
License terms: GPL 2.0

* src/rp2040/3rdparty/reSID16


### FAT Filesystem

```
Copyright (c) 2020-2021, Elehobica
```
License terms: BSD-2-Clause

* src/rp2040/3rdparty/fatfs


### PicoTerm

```
Copyright 2023 by S.Dixon & D.Meurisse
```
License terms: BSD-3-Clause

* src/rp2040/vga-term


### Raspberry Pi Pico Examples

On several occasions things were taken from Raspberry Pi Pico examples:
```
Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
```
License terms: BSD-3-Clause


## Special Thanks

- Benson of [Tristar & Red Sector inc.](https://trsi.org/) for a lot of
  contributions
- David Given, author of [CP/M for the 6502](https://github.com/davidgiven/cpm65)
    - provided an operating system for the Sorbus
    - helped on implementing the dhara wear leveling for the flash
- [Life with David](https://www.youtube.com/@LifewithDavid1)
    - especially for his [PIO examples](https://github.com/LifeWithDavid/Raspberry-Pi-Pico-PIO)
- Hans Otten
    - [http://retro.hansotten.nl](http://retro.hansotten.nl) is one of the best
      resources on SBCs from the 1970s and 1980s.
