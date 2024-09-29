Monitors
========

Not displays are meant here, but software that allows you to view and change
the content of memory. And most of the times, more.

On Target
=========

WozMon
------

The most basic one, implemented originally in just 256 bytes. Everything is
entered and displayed as hex.

Available commands (examples):

- `0400`: dump the content of memory location $0400
- `0400 0404`: dump the content of memory locations $0400 and $0404
- `0400.0407`: dump the content of memory locations $0400 to $0407
- `0400: 9C`: write $9C to memory location $0400
- `0400: 9C 01 DF A0 00 00 09 60`: write the bytes to memory starting at $0400
- `0400R`: start the program $0400

A significant change to the Sorbus version of WozMon is that if a routine
started using the `R` command returns using the `RTS` opcode will drop you
back to WozMon.


TIM
---

TIM (Terminal Interface Monitor) is a small (1kB) program that was part of the
MOS 6530-004 chip. That chip was sold together with the first 6502 CPUs and
documentation on how to build a computer with this. Also this chip is used in
the Jolt Computer, which was the first computer sold based on the 6502.

The following descriptions lacks description of the paper tape and other I/O
routines, besides mentioning them.

Output: upon start you are typically greeted with
```
    ADDR P  A  X  Y  S 
 *  E619 33 28 00 00 FF
.
```
The first line is an addition by the Sorbus version of TIM to help
understanding the output below:

- The first char indicates the kind of interrupt that occured:
  - ` ` (space): BRK instruction (BRK #$00 on Sorbus, since it's implemented
    as a two byte instruction
  - `#` (hash): NMI triggered via $FFFA/$FFFB
  - `%` (percent): IRQ triggered via $FFFE/$FFFF (Sorbus extension)
- `*` always printed
- ADDR: current address. The monitor is typically triggered by an interrupt.
- P: processor flags
- A: accumulator
- X: index register X
- Y: index register Y
- S: stack pointer (full stack pointer value is $01xx)

The following dot is the input prompt.

Available commands:

- `M`: dump 8 bytes of memory
- `R`: dump registers (the line described above)
- `:`: change the former output data
- `G`: go to the address shown in ADDR
- `L`, `W`, `H`: these are related to paper tape

So jumping to a specific address is handled by first running `R`, then changing
the ADDR with `:` (you can skip the rest by pressing return), and finally jump
using `G`. However, to return to the monitor you have to invoke it again using
the `BRK $00` opcode, `RTS` will not work here, like it did in WozMon.

A more detailed documentation is at the start of the
[TIM source code](../src/65c02/native_rom/tim.s).


On Meta-Mode
============

Pressing Ctrl+] will get you in a "Meta-Mode" where the CPU stops. Then
a debug menu is shown which provides some developer information. A few
entries there also relate to "monitor commands".

- B)acktrace: this will print out the last 512 accesses to the bus (aka
  CPU cycles). This option will also provide some disassembly. However,
  not every line is valid, as an instruction takes several cycles to
  execute (up to 8), and no logic to detect instruction fetches has been
  implemented, yet.
- D)isassemble: this will disassemble memory. Press space to advance.
  Press "Q" to quit and return to meta menu.
