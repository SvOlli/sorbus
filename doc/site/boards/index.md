
# The Sorbus Computer Boards

All boards are designed using [KiCAD](https://www.kicad.org/). Both the
generated data like schematics, gerber files and the projects themselves
are checked in as part of the source code, so you can fully recreate and
modify the project to your liking.

[Backplane](backplane.md), [65C02 CPU](65c02cpu.md) and
[Chipset](chipset.md) form a complete system, as can be seen below.

![The Sorbus Computer](../images/SorbusAssembled.jpg)

The [Junior](junior.md) is those three boards rolled into one, reducing
cost and solder work to create a base system.

[Sound](sound.md) and [VGA](vga.md) are optional cards to provide audio
and video capabilities. The [Chipset](chipset.md) has also been repurposed
to drive a [32x32 WS2812 LED Matrix](../jam/32x32display.md).

All Sorbus boards created so far:

- [Backplane](backplane.md)
- [65C02 CPU](65c02cpu.md)
- [Chipset](chipset.md)
- [Junior](junior.md)
- [Sound](sound.md)
- [VGA](vga.md)
