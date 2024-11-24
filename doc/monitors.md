Monitors
========

Not displays are meant here, but software that allows you to view and change
the content of memory. And most of the times, more.

On Target
=========

Sorbus System Monitor
---------------------

This is intended to be the "go-to" system monitor to use. There are two
different versions to invoke:
- NMOS 6502 version in Bootsector 2, as part of "NMOS 6502 toolkit".
- the standard version included in ROM and invoked either by pressing "m" in
  the reset menu or by invoking BRK #$0e (or BRK #$00, if user vector UVBRK
  wasn't changed)

Supported commands:

| CMD | Function                             |
| --- | ------------------------------------ |
| R   | dump shadow registers                |
| G   | go                                   |
| ~   | edit shadow registers                |
| M   | dump memory                          |
| :   | edit memory                          |
| D   | disassemble                          |
| A   | assemble                             |
| ;   | handle papertape input               |
| >   | (edit disassmbly not used right now) |

Differences between 65SC02 in ROM and NMOS 6502 toolkit versions

| Topic   | Native ROM                          | NMOS 6502 toolkit                 |
| ------- | ----------------------------------- | --------------------------------- |
| opcodes | CMOS 65SC02 opcodes                 | NMOS 6502 opcodes                 |
| BRK     | CPU correct 2-Byte BRK #$xx         | adjusted to historical 1-Byte BRK |
| storage | save to/load from CP/M-fs           | not supported                     |
| IRQ     | handled via kernel to check for BRK | passed almost directly to monitor |
| banking | bank shadow register used           | bank shadow register ignored      |

WozMon
------

The most basic one, implemented originally in just 256 bytes for the Apple 1
Computer. Everything is entered and displayed as hex.

Available commands (examples):

- `0400`: dump the content of memory location $0400
- `0400 0404`: dump the content of memory locations $0400 and $0404
- `0400.0407`: dump the content of memory locations $0400 to $0407
- `0400: 9C`: write $9C to memory location $0400
- `0400: 9C 01 DF A0 00 00 09 60`: write the bytes to memory starting at $0400
- `0400R`: start the program $0400 (an alias `0400G` can be used as well)

A significant change to the Sorbus version of WozMon is that if a routine
started using the `R` command returns using the `RTS` opcode will drop you
back to WozMon.


System Monitor of Apple //c (WozMon 2c)
---------------------------------------

This monitor is a very enhanced version of the original WozMon, now
utilizing about 1.5k bytes without the help message. But for this it
comes with a lot of new features like a disassembler, a direct mini
assembler from the Apple //c and file access which is custom to the
Sorbus Computer.

As of now, this monitor needs to be loaded from the internal drive
using the file browser. This is intended to change, once it is
considered stable.

Displaying and entering data works the same as on the original WozMon.
There is an addition: instead of entering hex data, you can also enter
ASCII by prepending the letter with a single quote (`'`). However, when
entering multiple data, make sure that every letter is prepended with a
quote, and also that the input is separated by spaces.

Furthermore, the `R` command has been renamed to `G` for "go".

All implemented commands (examples):

When the prompt is a asterisk (`*`):

- `0400`: dump the content of memory location $0400
- `0400 0404`: dump the content of memory locations $0400 and $0404
- `0400.0407`: dump the content of memory locations $0400 to $0407
- `0400: 9C`: write $9C to memory location $0400
- `0400: 9C 0D 03 A0 00 00 09 60`: write the bytes to memory starting at $0400
- `0410: 20 06 FF 'H 'e 'l 'l 'o 'r 'l 'd '! 00 60`:
   write a small program that outputs "Hellorld!". Greetings to Usage Electric.
- `0400G`: start the program $0400
- `50+50`: add those to hex values, getting $A0
- `30-22`: subtract those to hex values, getting $0E
- `R`: print out the CPU shadow registers of A, X, Y, P and SP. These will
   be read when using `G` and saved to then the program exists with an `RTS`
   opcode. (Option renamed from Ctrl-E.) The values of those registers can
   be changed by the `:` command as the edit address pointer has been put to
   the correct place in memory ($00FB-$00FF).
- `c000<0400.0bffM`: move (or rather copy) the memory from $0400 to $0BFF to
   $C000 to $C7FF.
- `c000<0400.0bffV`: verify (or rather compare) the memory from $0400 to
   $0BFF to $C000 to $C7FF.
- `bd<c000.cfffP`: put (or rather fill) the memory from $C000 to $CFFF with
   the value of $BD.
- `0400L`: list (or rather disassemble) 20 instructions starting at $0400.
- `!`: drop to the direct mini assembler (see below)
- `A$`: display the directory of the user id $A (10) (Sorbus extension)
- `A<1000{int-test.sx4`: load the file "int-test.sx4" from user id $A (10)
   to address $1000.
- `2<1000.1FFF}savetest.bin`: load the file "savetest.bin" from user id $2
   (2) from address $1000 to $1FFF, including.

When the prompt is an exclamation mark (`!`), the direct mini assembler is
running. Now the user input is evaluated like:

- `1000:stz $DF01`: assemble the instruction `STZ $DF01` to address $1000.
   (Note the `$` is optional.)
- ` rts`: assemble the instruction `RTS` to the now current address.
   ($1003 in this example, also note the mendatory leading space character
   for this case.)
- an empty input returns to the "asterisk input"

The supported instruction set is of the 65SC02. The BRK instruction is not
supporing an operand here, as it is used with the Sorbus Native kernel.


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

Pressing Ctrl+] will get you in a "meta-mode" where the CPU stops. Then
a debug menu is shown which provides some developer information. A few
entries there also relate to "monitor commands".

- B)acktrace: this will print out the last 512 accesses to the bus (aka
  CPU cycles). This option will also provide some disassembly. However,
  not every line is valid, as an instruction takes several cycles to
  execute (up to 8), and no logic to detect instruction fetches has been
  implemented, yet. But there are some rules installed that disables
  the output of more false instructions.
- D)isassemble: this will disassemble memory. Press space to advance.
  Press "Q" to quit and return to meta menu.
- M)emory dump: view memory 256 bytes at a time, combined hexdump and
  ASCII.
