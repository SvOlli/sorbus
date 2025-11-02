
# Apple Computer 1 Emulation Core

This is a replica of the
[Apple Computer 1](https://en.wikipedia.org/wiki/Apple_I)
with enhanced features.

*Development of this core has been stopped as it is considered feature
complete.* The [Sorbus JAM](jam/index.md) is considered a successor. 

Enhancements as compared to an original Apple 1 are:

- 64k of memory
- preloaded with [Krusader](https://github.com/st3fan/krusader)
- added routines preloaded in RAM upon startup
    - 2F0: print all characters
    - 280: detect CPU type (6502, 65C02 and 65816)

Furthermore, it uses a terminal instead of Woz's video circuitry. So no
blinking "@" as a cursor.


## Meta-Keys

A few keys are utilized for meta proposes, since they were not available
on an original Apple Computer 1:

| Key | Function |
| --- | -------- |
| `` ` `` | reset |
| `~` | clear screen |
| `{` | disable original terminal output delay (fast mode) |
| `}` | enable original terminal output delay |
| `|` | (future expansion) |


## Using an Apple Computer 1

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

### Using WozMon

WozMon knows four operations: read bytes, read ranges, write and execute.
The first line of a box is what is entered, the rest is the output by WozMon.

Dump a single byte of memory:

```
FF00
FF00: D8
```

Dump more that one byte at a time
```
FF00 FF03
FF00: D8
FF03: 7F
```

Dump a range of bytes of memory:
```
FF00.FF0F
FF00: D8 58 A0 7F 8C 12 D0 A9
FF08: A7 8D 11 D0 8D 13 D0 A9
```

Write to memory:
```
0: 53 56 4F 4C 4C 49
0000: 05
```

The output is from the dump command implied before the colon. Let's check
if our write to memory was successful:
```
0.5
0000: 53 56 4F 4C 4C 49
```

Without an address, data gets appended. First set pointer to memory by writing
a single byte:
```
6:20
0006: 00
```

Now append another one:
```
:21
```

And let's verify the full memory dump:
```
0.7
0000: 20 56 4F 4C 4C 49 20 21
```

And we've just written " HELLO !" to the first 8 bytes to memory. However,
the Apple 1 expects the bit 7 to be set to 1 for output.

Execute a program in memory by appending to "R" the address
```
FF00 R
FF00: D8\
```

Since we've just jumped back into WozMon, nothing interesting has happend
besides the output of the backslash.

## Caveats

- The system operates at a higher clockspeed as the original Apple 1:
  ~1.25MHz (Sorbus) vs ~0.95MHz (original).
- Once the input buffer is filled up, the meta keys will not be evaluated
  anymore. A power cycle is then required to reset the machine, if the
  computer does not fetch any keys from the queue. Since this is a known
  problem, the queue has been set to be 256 keys.
