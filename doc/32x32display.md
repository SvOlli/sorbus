
Sorbus 32x32 LED Display
========================


I/O Registers
-------------

I/O utilizes the page $D3xx. All registers are write only, as the hardware
only sniffes the bus and never actively drives it.

- $D300: DMA into framebuffer and flush (strobe register)
- $D301: DMA into framebuffer without flush (strobe register)
- $D302: transparency DMA info framebuffer and flush (strobe register)
- $D303: transparency DMA info framebuffer without flush (strobe register)
- $D304/5: start address of RAM source
- $D306/7: startpixel of framebuffer target (10 bit, absolute address)
- $D308/9: startpixel of framebuffer X/Y format (5 bit each)
- $D30A: width of DMA in pixels-1 (5 bit, $00=one pixel, $1f sets a full line)
- $D30B: height of DMA in pixels-1 (5 bit, $00=one line, $1f sets full height)
- $D30C: linestep of source in pixels-1 (8 bit)
- $D30D: transparency color for $D302/3
- $D30E: colormap / brightness (bits 7-6)
- $D30F: custom colormap data stream

(defaults are starting at $d304: $00,$cc,$00,$00,$00,$00,$1f,$1f,$1f,$00,$00)

If not all bits of a register are required, those will be masked out/ignored.

Since the board just sniffes the bus, DMA from internal drive is not
detected.
