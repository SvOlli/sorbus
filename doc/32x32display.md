
Sorbus 32x32 LED Display
========================


I/O Registers
-------------

I/O utilizes the page $D3xx. All registers are write only, as the hardware
only sniffes the bus and never actively drives it.

- $D300: DMA into framebuffer and flush (strobe register)
- $D301: DMA into framebuffer without flush (strobe register)
- $D302/3: start address of RAM source
- $D304/5: startpixel of framebuffer target (10 bit, absolute address)
- $D306/7: startpixel of framebuffer X/Y format (5 bit each)
- $D308: width of DMA in pixels-1 (5 bit, $00=one pixel, $1f sets a full line)
- $D309: height of DMA in pixels-1 (5 bit, $00=one line, $1f sets full height)
- $D30A: linestep of source in pixels-1 (8 bit)
- $D30B: transparency color
- $D30C: colormap / brightness? (bits 7-6)
- $D30D: custom colormap id register (valid value: 1-3)

- $D310: red color for custom colormap
- $D311: green color for custom colormap
- $D312: blue color for custom colormap

(defaults are starting at $d302: $00,$cc,$00,$00,$00,$00,$1f,$1f,$1f,$00,$00)

If not all bits of a register are required, those will be masked out/ignored.

Value written do $D300/1 indicades mode (can be or'ed together):
- $00: plain copy
- $01: transparency: no copy when source color = transparent
- $02: transparency: no copy when destination color = transparent
- $04: transparency: only copy when destination color = transparent
- $06: do nothing

Color palettes:
- $00: Insane's RGBI2222
- $01: Custom 1
- $02: Custom 2
- $03: Custom 3
- $04: C64 like
- $05: C16 like
- $06: Atari 8-bit like
- $07: Atari 8-bit like, ordered differently

Custom defined color palettes are always specified in RGB444. Writing to $D30D
resets internal index of $D310-$D312 to $00, when a colorvalue is written,
lower 4 bits will be taken as value for this index, and the index will be
incremented. Once $100 color values has been written, that color slot does not
take new color values. If a value other than 1-3 is written to $D30D, then
$D310-$D312 will not accept any data as well.

Since the board just sniffes the bus, DMA from internal drive or swap pages is
not detected.
