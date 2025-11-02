
# MCP

*Development of this core has been stopped as it is considered feature
complete.* The [Sorbus JAM](../jam/index.md) is considered a successor. 

MCP stands for Monitor Command Prompt, because it describes what it is,
and the way it treats the CPU, it's also a [Master Control
Program](https://en.wikipedia.org/wiki/List_of_Tron_%28franchise%29_characters#Master_Control_Program).

Loosely based on monitors as known from 6502 based Commodore Computers.
The system can be run at clock speeds between 1Hz and 0.1MHz.

The system is typically stopped using the RDY pin. Using the `s` command
the system can be set into "running" mode for a specified number of
clock cycles.


## Commands

| Command | Description                                              |
| ------- | -------------------------------------------------------- |
| `help`  | display help                                             |
| `cold`  | fully reinitialize system ("debug")                      |
| `sys`   | show system information (CPU, flash)                     |
| `freq`  | set frequency (dec of 1-100000)                          |
| `dis`   | enable (on)/disable (off) automated disassembly          |
| `bank`  | enable (on)/disable (off) 65816 banks, select bank (dec) |
| `reset` | trigger reset (dec number of cycles)                     |
| `irq`   | trigger maskable interrupt (dec number of cycles)        |
| `nmi`   | trigger non maskable interrupt (dec number of cycles)    |
| `:`     | write to memory &lt;address> &lt;value&gt; .. (hex)      |
| `f`     | fill memory &lt;from&gt; &lt;to> &lt;value&gt; (hex)     |
| `m`     | dump memory (&lt;from&gt; (&lt;to&gt;)) (hex)            |
| `s`     | run number of steps (dec number of cycles)               |

All commands are case sensitive.
