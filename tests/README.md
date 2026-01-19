# Sorbus Testing Framework

A testing framework that runs the RP2040 chipset code using https://github.com/c1570/rp2040js/ and wires it up with a 65C02 CPU card emulation based on https://github.com/floooh/chips .

## Running

`run_tests.sh`

## Architecture

The framework consists of:

1. **CPU Card Emulation (C code)**: `cpu_card_wrapper.c` - Wraps the m65c02 emulator to provide a simple tick() interface
2. **FFI Bridge**: Node.js module using `koffi` library to call C functions from JavaScript
3. **Test Runner**: `test_runner.ts` - Main emulator that:
   - Loads RP2040 firmware
   - Initializes RP2040 via rp2040js
   - Connects RP2040 GPIO pins to the 65C02 CPU card
   - Runs the emulation loop
