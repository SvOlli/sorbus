
Sorbus 32x32 LED Display
========================


I/O Registers
-------------

I/O utilizes the page $D3xx. All registers are write only, as the hardware
only sniffes the bus and never actively uses is.

- $D300: DMA into framebuffer and flush (strobe register)
- $D301: DMA into framebuffer without flush (strobe register)
- $D302/3: start address of RAM source
- $D304/5: startpixel of framebuffer target (10 bit, absolute address)
- $D306/7: startpixel of framebuffer X/Y format (5 bit each)
- $D308: width of DMA in pixels-1 (5 bit, $1f sets a full line)
- $D308: height of DMA in pixels-1 (5 bit, $1f sets full height)
- $D30A: linestep of source in pixels-1 (5 bit)
- $D30B: linestep of destination in pixels-1 (8 bit)
- $D30C: colormap / brightness
- $D30D: custom colormap data stream

(defaults are starting at $d302: $00,$cc,$00,$00,$00,$00,$1f,$1f,$1f,$1f,$00)
Since the board just sniffes the bus, DMA from internal drive is not
detected.
