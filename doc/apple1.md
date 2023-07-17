Apple Computer 1 Emulation Core
===============================

This is a replica of the
[Apple Computer 1](https://en.wikipedia.org/wiki/Apple_I)
with enhanced features:
- 64k of memory
- preloaded with [Krusader](https://github.com/st3fan/krusader)
- added routines preloaded in RAM upon startup
  - 2F0: print all characters
  - 280: detect CPU type (6502, 65C02 and 65816)

Meta-Keys
---------
A few keys are utilized for meta proposes, since they were not available
on an original Apple Computer 1:
- "`": reset
- "~": clear screen
- "{": disable original terminal output delay
- "}": enable original terminal output delay
- "|": (future expansion)

Using an Apple Computer 1
-------------------------
This will be explained more in detail on it's
[dedicated web page](https://xayax.net/sorbus/examples_apple1.php). But
here are a few hints how to use WozMon:
- After pressing reset ("`"), WozMon will output a backslash followed by
  a new line
- you can dump the contents of memory just by entering the hex number
  of that address: FF00
- you can dump ranges of memory by using the dot (".") FF00.FFFF
- you can jump to an address by suffing the number with an "R": FF00R
- since BASIC is preloaded with Krusader, you can run it using
  - E000R (cold start)
  - E2B3R (warm start without clearing the program)

Caveats
-------
The system operates at a higher clockspeed as the original Apple 1:
~1.25MHz (Sorbus) vs ~0.95MHz (original).

Once the input buffer is filled up, the meta keys will not be evaluated
anymore. A power cycle is then required to reset the machine, if the
computer does not fetch any keys from the queue. Since this is a known
problem, the queue has been set to be 256 keys.
