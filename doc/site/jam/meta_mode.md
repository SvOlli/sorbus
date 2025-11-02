
# Meta-Mode

Pressing Ctrl+] will get you in a "meta-mode" where the CPU stops. Then
a debug menu is shown which provides some developer information. A few
entries there also relate to "monitor commands".


## Backtrace 

Pressing `B` will print out the last 512 accesses to the bus (aka
CPU cycles). This option will also provide some disassembly. However,
not every line is valid, as an instruction takes several cycles to
execute (up to 8), and no logic to detect instruction fetches has been
implemented, yet. But there are some rules installed that disables
the output of more false instructions.

Also note that the disassembly function cannot use the SYNC pin, for this
it has to guess when a new instruction is executed. This is not always
100% accurate.


## Disassemble

Pressing `D` and entering a memory address will disassemble memory.
Press space to advance. Press `Q` to quit and return to meta menu.


## Memory Dump

Pressing `M` and entering a memory address will show memory 256 bytes
at a time, combined hexdump and ASCII.


## Upload

Pressing `U` allows to upload a file to memory using the
[XModem](https://en.wikipedia.org/wiki/XMODEM) protocol.
