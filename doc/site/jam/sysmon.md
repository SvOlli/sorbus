
# Sorbus System Monitor

This is intended to be the "go-to" system monitor to use. There are two
different versions to invoke:

- NMOS 6502 version in Bootsector 2, as part of "NMOS 6502 toolkit".
- the standard version included in ROM and invoked either by pressing "m" in
  the reset menu or by invoking BRK #$0e (or BRK #$00, if user vector UVBRK
  wasn't changed)


## Invoking

When the System Monitor is started it looks like this:

```txt
Sorbus System Monitor via menu
   PC  BK AC XR YR SP NV-BDIZC
 ~FFF2 01 00 00 00 FF 00100110
>_  <-- cursor
```

Note how it tells you that it was launched via the menu. This can be also
an IRQ, NMI or BRK, if the vectors for those were not changed from the
system default. The line starting with "PC" is just information on what is
shown below:

| Entry    | Meaning                                                                           |
| -------- | --------------------------------------------------------------------------------- |
| PC       | Program Counter                                                                   |
| BK       | BanK (not a CPU register but a system register) (not available on NMOS version)   |
| AC       | ACcumulator                                                                       |
| XR       | X index Register                                                                  |
| YR       | Y index Register                                                                  |
| SP       | Stack Pointer                                                                     |
| NV-BDIZC | processor flags: Negative, oVerflow, Brk, Decimal, Interrupt disable, Zero, Carry |

Those registers are not the actual CPU registers, but shadow registers to
which the data was copied upon startup. When leaving the monitor with the
go (`G`) command, the data will be written back to those registers. The
tilde (`~`) just denotes that the following line contains registers. The
greater-than-sign (`>`) is the entry prompt.


## Line Input

The line input is capable of minimal command line editing

| Key       | Function                                |
| --------- | --------------------------------------- |
| Home      | go to start of input                    |
| Ctrl+A    | go to start of input                    |
| End       | go to end of input                      |
| Ctrl+E    | go to end of input                      |
| Backspace | delete character left of cursor         |
| Delete    | delete character under the cursor       |
| Ctrl+D    | delete character under the cursor       |
| Ctrl+K    | delete everything right from the cursor |
| Ctrl+C    | cancel input                            |

(This is a kernel function that also used by OSI BASIC for example.)


## Supported Commands

| Command | Function                          | Comment                         |
| ------- | --------------------------------- | ------------------------------- |
| R       | dump shadow registers             |                                 |
| G       | go                                |                                 |
| ~       | edit shadow registers             |                                 |
| M       | dump memory                       |                                 |
| :       | edit memory                       |                                 |
| D       | disassemble                       |                                 |
| A       | assemble                          |                                 |
| C       | compare                           |                                 |
| F       | fill                              |                                 |
| H       | hunt (find)                       |                                 |
| T       | transfer (copy)                   |                                 |
| ;       | handle papertape input            | (not available on CMOS version) |
| BR      | read sector from internal drive   |                                 |
| BW      | write sector to internal drive    |                                 |
| L       | load file from CP/M filesystem    | (not available on NMOS version) |
| S       | save file to CP/M filesystem      | (not available on NMOS version) |
| $       | show directory of CP/M filesystem | (not available on NMOS version) |

(Input is case insensitive.)


### Command Examples

The available commands are better explained by examples then some abstract
syntax notation. Also is the behaviour very close related to monitors related
to Commodore, like the monitor in the C264 series, the C128 or those which
are known from cartridges like Action Replay or Final Cartridge III.

Addresses and data are always entered in hex. Addresses need to specified with
1 to 4 digits, data with 1 or 2 digits. When addresses are specified with
4 digits and data with 2 digits, spaces are optional. In the examples those
are included for better readability.

Note that an end address is specified "the Commodore way" as "plus 1", so the
parameters "0400 0480" will only span the area starting at $0400 and using
$047f as the last byte.

So these commands are equivalent:

```txt
>m 0400 0480
>m04000480
>m 400 480
```


#### Hexdump memory

```txt
>M 0400 0480
>m 0400 0480
>m04000480
>m 400 480
```

Those command all are equivalent and show the memory from 0400 to 047f:

```txt
 :0400  A2 00 A0 00 B9 31 04 9D  00 02 C8 C0 0B 90 02 A0  .....1..........
 :0410  00 E8 D0 F0 A9 E2 20 03  FF A9 94 20 03 FF AE 02  ...... .... ....
 :0420  DF BD 00 02 20 03 FF 20  00 FF C9 03 D0 E6 6C FC  .... .. ......l.
 :0430  FF 80 82 8C 90 94 98 9C  A4 AC B4 BC 00 00 00 00  ................
 :0440  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
 :0450  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
 :0460  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
 :0470  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
```

So spaces are optional as long as you use 4 digits for addresses and 2 digits
for data. After running this you can scroll up/down through memory by
pressing up/down on the keyboard:

```txt
>:0470 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
```

ASCII output is missing here. Memory can be edited here.


#### Disassemble

```txt
>d e000
>d e000 e100
```

The first one disassembles 20 instructions. You can enter `d` without an
address again to continue disassembling for another 20 instructions. The
second one will disassemble the memory area from $e000 to $e100.


#### Assemble

```txt
>a 0400 jmp (fffc)
```

It will turn to this for a successful assembly:

```txt
 > 0400   6C FC FF    JMP   ($FFFC)
>A 0403               _  <-- cursor
```

See how the assembler has already prepared the next line for entering.


#### Compare

```txt
>c 1000 2000 4000
```

Compares the area from $1000 to $1fff with the area from $4000 to $4fff.
All addresses that do not match will be printed out, where the address of
the first area will be used.


#### Fill

```txt
>f 1000 2000 bd
```

Fills the memory from $1000 to $1fff with the value of $bd.


#### Hunt

```txt
>h 0400 1000 f2 ff
```

Searches for the byte combination $ff, $f2 in the area from $0400 to $0fff.
When such a combination is found, the address of the first byte is printed.


#### Transfer

```txt
>t 1000 2000 c000
```

Transfer (or rather copy) the memory area from $1000 to $1fff to the area of
$c000 to $cfff.

Note that data wil be copied from start to end address, so

```txt
>t 1000 2000 1001
```

will write the byte at $1000 to every location until $2000.

#### Block Read/Write

```txt
>br 0040 0400
>br 40 400
```

Will read a 128 bytes sector from the internal drive to memory. In this
case the first sector of the second bootblock containing the NMOS 6502
toolkit. This internal drive has a capacity of 4MB in 32768 sectors from
$0000 to $7fff.

Technically we're using sectors of 128 bytes here, not blocks combining
16 sectors (2048 bytes) to a single block. But the `S` is already assigned
to "Save". (See below.) Also, the commands `BR` and `BW` were used by the
monitor of the Action Replay Cartridge as well.


#### CP/M Filesystem Load/Save

```txt
>la asmprint.sx4 0400
>l a asmprint.sx4 0400
>s2 memory.bin 1000 2000
>s 2 memory.bin 1000 2000
```

Loads the file "asmprint.sx4" to memory location $0400 from the user-id
$A (or 10 decimal). For save you also need to add an end address. This is
shown in the third and forth example where user id $2 is used. Again, the
addresses are specified in "Commodore format" excluding the last byte, so
"1000 2000" saves the memory from $1000 to $1fff.

Note that the filesystem implementation always works on full sectors, so
even a save like "1000 1001" will only save a single byte (also noted as
this in the directory), it will actually save (and load) the area from
$1000 to $107f.

Also note, while spaces are typically optional in the monitor, to separate
the filename from the rest of the input, they are essential.

Since kernel routines are used, load and save will always use RAM. Supported
areas are $0400-$cfff and $df80-$ffff. $0004-$03ff will do unpredictable
stuff, as this area is swapable. $d000-$df7f will just cause an error.


#### Directory

```txt
$A
```

Will show contents of the directory of the user partition $A.


## Differences between 65SC02 in ROM and NMOS 6502 toolkit versions

| Topic   | JAM ROM                             | NMOS 6502 toolkit                 |
| ------- | ----------------------------------- | --------------------------------- |
| opcodes | CMOS 65SC02 opcodes                 | NMOS 6502 opcodes                 |
| BRK     | CPU correct 2-Byte BRK #$xx         | adjusted to historical 1-Byte BRK |
| storage | save to/load from CP/M-fs           | not supported                     |
| IRQ     | handled via kernel to check for BRK | passed almost directly to monitor |
| banking | bank shadow register used           | bank shadow register ignored      |

If the bank register is set to $00, then using "G" command will copy the
BIOS from ROM to RAM in process. Without it the code for executing would
not work.

Also upon startup of the NMOS 6502 toolkit, you will be greeted with the
[CPU detected](../opcodes/cpu_detect.md) upon cold boot. It should be one of the following:

```txt
6502 CPU features: NMOS
65SC02 CPU features: CMOS
65C02 CPU features: CMOS BBSR
65CE02 CPU features: CMOS BBSR Z-Reg
65816 CPU features: CMOS 16bit
6502RA CPU features: NMOS
```
