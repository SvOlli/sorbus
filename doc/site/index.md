
# The Sorbus Computer

The Sorbus Computer is a small, cheap, and expandable computer based upon
the [6502](https://en.wikipedia.org/wiki/MOS_Technology_6502) or rather
the [65C02](https://en.wikipedia.org/wiki/WDC_65C02) CPU and an
[RP2040](https://en.wikipedia.org/wiki/RP2040) microcontroller from
Raspberry Pi. It is fully open source, the [source code is available
at GitHub](https://github.com/SvOlli/sorbus).

The means that even the hardware is for the most part software written
in C. The kernel for the 65C02 is written using the ca65 assembler from
the [cc65](https://github.com/cc65/cc65) cross-compiler suite.

While the original implementation of the machine was distributed among
three PCBs, the current iteration (called the Junior) has merged those
three boards into a single one.

![Sorbus Junior](images/SorbusJunior.jpg)

## What can it be used for?

It's good for a couple of things. Even more than I expected, when I
first built this machine. Here are a few suggestions:


### Understanding The 6502 Processor Family

There is a [course devided into a couple of sessions](mcp/lessons.md)
that takes a dive into understanding how the CPU works by looking at
what happens on the bus during each clock cycle. For this a tailored
firmware (core) called [MCP](mcp/index.md) has been written.


### Running The Replica Of An Apple 1 Computer

Ever wanted to know, how you would have worked with an Apple 1? Here
you can [try this out](apple1.md). It's more cumbersome that you'd
probably think.


### Implement Your Own Design Of An 8-Bit Computer

A lot of example code is already available. And this is more fun, than
you'd probably think. I know, because I did this


### Running The Sorbus JAM Core To Have A Capable 8-Bit Computer

The Sorbus JAM is the core designed especially for this hardware. It
includes a 4MB mass storage device utilizing the flash of the RP2040
board. Ir's capable of running
[CP/M 65](https://github.com/davidgiven/cpm65),
[TaliForth2](https://github.com/SamCoVT/TaliForth2),
[Microsoft BASIC 1.0](https://github.com/mist64/msbasic), several
machine language monitors, and a couple of other things.

It can also run [demos](https://en.wikipedia.org/wiki/Demoscene) as you
can see here: [1k LEDs Is No Limit](https://xayax.net/1k_leds_is_no_limit/).


## Why The Name "Sorbus"?

I liked following the tradition to name it after a fruit. Think about it:
it uses a Raspberry (Pi) and can run like an Apple (1). However most
fruits were already "taken". So, then I remembered the German
"Vogelbeeren" that were around in my childhood. My mother always told me:
"Do not eat those, they are poisonous!" She was a bit exaggerating there,
as the "only" make you sick in the stomach, and don't cause and serious
health issue you. However, I still played with them, throwing them around
and other things that kids do.

So, this is a very fitting name for this computer: it's something you play
with, but not for the "primary use" of a computer, like word processing.
I looked up for a good translation and found
[Sorbus](https://en.wikipedia.org/wiki/Sorbus). Also, here is an [image
provided by Sir Garbagetruck](images/SorbusPlant.jpg).
