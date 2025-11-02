
# Useful Software


## About

The BSD 2 clause (and other) license agreements state that

> Redistributions in binary form must reproduce the above copyright notice,
> this list of conditions and the following disclaimer in the documentation
> and/or other materials provided with the distribution.

This program is implementing this requirement. Technically, this program is
a bit interresting, because the text is compressed within the executeable.

- source code at `src/65c02/jam/about/`


## Clock

A small example on how to use the clock timer. Just enter the time,
and it will update the current time.

But it is damn hard to keep the time for just a day. Tried it on a KIM-1
clone ([PAL-1](http://retro.hansotten.nl/6502-sbc/kim-1-manuals-and-software/my-kim-1-family/kim-replicas-and-clones/pal-1/))
and failed miserably. On the Sorbus, this works a bit better, but it still
runs a bit faster, so over the timespan of a day you will notice it.

- source code at `src/65c02/jam/sx4/clock.s`


## Instant Assembler

written by Alan Cashin

The assembler is presented more for curiosity value than as a serious
programming tool. It was written for the base model KIM-1 with very little
memory, to ease the task of entering programs.

Due to the small footprint, there is no error checking. It is up to the
programmer to ensure only valid instructions are entered.

All alphabetics are entered in upper case only. All data (addresses, immediate,
etc) is entered as hex digits. One useful facility is branch instructions take
the absolute address and calculate the relative address.

Some input is accepted as soon as the last character is input and does not
require a terminating character. This applies to implied instructions (that
have no operand) and byte input (#hh). E.g. if TAX is input, as soon as the X
is input the assembler generates the code and prompts for the next instruction.

Where an operand is required, the input of the operand is completed by entering
a character other than one of [hex , # ( ) X Y]. Conventionally that character
is a space. Control characters are best avoided.

Where an operand is required, the input of the operand is completed by entering
a character other than one of [hex , # ( ) X Y]. Conventionally that character
is a space. Control characters are best avoided.

operands take the form (where h is a hexadecimal digit):

- `A` - accumulator (for ROL etc.)   hh - zero page address  #hh - immediate
- `hh,X`  `hh,Y` - zero page indexed   hhhh - absolute address
- `hhhh,X`  `hhhh,Y` - absolute indexed  (hh,X)  (hh),Y - indexed indirect
- `(hhhh)` - absolute indirect NOTE: branches require an absolute address

An entry address is input as `*hhhh<sp>` (note the space at the end).
This can be input at any time.

Other valid input:

- `#hh` generates the byte hh
- `<` cancels the current input (for instance, if a mistake is made)
- `/` exits the assembler

It is suggested to start the entered program using
WozMon's `R` command. Assembler can be restarted with `0400 R`

- source code at `src/65c02/jam/sx4/inst-ass.s`

(This is superseded by [system monitor](sysmon.md).)


## Kernel Interrupt Tests

This program provides testcases for the following kernel interrupt
service routines:

- $09: directory
- $0a: VT100
- $0c: line input (editing and entering new text)
- $0d: generate sine
- $11: decimal print
- $0e: system monitor

Also those implementations can be used as a reference how to use them.

- source code at `src/65c02/jam/sx4/int-test.s`


## Tali Forth 2

From the README of [Tali Forth 2](https://github.com/SamCoVT/TaliForth2):

> Tali Forth 2 is a subroutine threaded code (STC) implementation of an
> ANS-based Forth for the 65c02 8-bit MPU. The aim is to provide a modern
> Forth that is easy to get started with and can be ported to individial
> hardware projects, especially Single Board Computers (SBC), with little
> effort. It is free -- released in the public domain -- but with absolutely
> no warranty of any kind.

Special thanks to the guys from the [Steckschwein](https://steckschwein.de/),
since their port was the blueprint for this port.

However, Tali Forth 2 does not support any kind of mass storage. Typically,
any kind of program is copy/pasted into the terminal for uploading.

As with CP/M, only a binary is imported here for usage. For building and
integration, the script `src/tools/external-taliforth2.sh` is provided.
