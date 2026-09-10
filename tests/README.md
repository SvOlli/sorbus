# Sorbus Testing Framework

A testing framework that runs the RP2040 chipset code using a C transpile of
https://github.com/c1570/rp2350js/ (see `rp2350js-c.h`) and wires it up with a
65C02 CPU card emulation based on https://github.com/floooh/chips (`m65c02.h`).

## Running

`run_tests.sh` (requires the firmware to be built first: `make all` in the top
level directory creates `build/rp2040/jam_alpha_picotool.uf2`).

## Architecture

1. **Emulator**: `rp2350js-c.h` - the rp2040js/rp2350js emulator
2. **Test Runner**: `test_runner.c` - Main emulator that:
   - Loads RP2040 firmware (UF2)
   - Initializes RP2040 via the transpiled emulator
   - Connects RP2040 GPIO pins to the 65C02 CPU card
   - Bridges the emulated USB CDC to stdin/stdout for the jam console
   - Runs the emulation loop
