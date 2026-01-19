#!/usr/bin/env node
/**
 * Sorbus Testing Framework
 *
 * This test framework runs the RP2040 chipset code using rp2040js
 * and wires it up with the 65C02 CPU card emulation.
 *
 * GPIO Pin Mapping:
 * - Bits 0..15:   Address (A0-A15)
 * - Bits 16..23:  Data (D0-D7)
 * - Bit 24:       RW line (low: write)
 * - Bit 25:       Clock line
 * - Bit 26:       RDY line (CPU halts on low)
 * - Bit 27:       IRQ line (active low)
 * - Bit 28:       NMI line (active low)
 * - Bit 29:       Reset line (active low)
 */

const MAX_CYCLES = -1;
const DEBUG = 0;
const RP_MHZ = 125;

import { RP2040, USBCDC, GPIOPinState } from './rp2040js/dist/esm/index.js';
import { bootromB1 } from './rp2040js/demo/bootrom.js';
import * as fs from 'fs';
import koffi from 'koffi/index.js';
import * as path from 'path';
import { fileURLToPath } from 'url';
import { decodeBlock } from 'uf2';

// Polyfill __dirname for ES modules
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// UF2 loading helper
const FLASH_START_ADDRESS = 0x10000000;
function loadUF2(filename, rp2040) {
  const file = fs.openSync(filename, 'r');
  const buffer = new Uint8Array(512);
  while (fs.readSync(file, buffer) === buffer.length) {
    const block = decodeBlock(buffer);
    const { flashAddress, payload } = block;
    rp2040.flash.set(payload, flashAddress - FLASH_START_ADDRESS);
  }
  fs.closeSync(file);
}

const FIRMWARE_PATH = `${__dirname}/../build/rp2040/jam_alpha_picotool.uf2`;
const INITIAL_PC = 0x10000000;

// Load the CPU card library
const libraryPath = path.join(__dirname, 'libcpu_card.so');
const lib = koffi.load(libraryPath);

const cpu_card_init = lib.func('void cpu_card_init()');
const cpu_card_get_pins = lib.func('uint64_t cpu_card_get_pins()');
const cpu_card_tick = lib.func('uint64_t cpu_card_tick(uint64_t input_pins)');

// Pin masks (matching bus.h)
const MASK_ADDRESS  = 0x0000FFFFn; // Bits 0-15
const MASK_DATA     = 0x00FF0000n; // Bits 16-23
const MASK_RW       = 0x01000000n; // Bit 24
const MASK_CLOCK    = 0x02000000n; // Bit 25
const MASK_RDY      = 0x04000000n; // Bit 26
const MASK_IRQ      = 0x08000000n; // Bit 27
const MASK_NMI      = 0x10000000n; // Bit 28
const MASK_RESET    = 0x20000000n; // Bit 29
const MASK_SYNC     = 0x40000000n; // Bit 30
const SHIFT_DATA    = 16n;

// Initialize the CPU card
console.log('Initializing 65C02 CPU card...');
cpu_card_init();
const initialCpuPins = BigInt(cpu_card_get_pins());
console.log(`Initial CPU pins: 0x${initialCpuPins.toString(16)}`);

// Initialize RP2040
console.log('Initializing RP2040...');
const mcu = new RP2040();
mcu.loadBootrom(bootromB1);

// Load firmware
if (fs.existsSync(FIRMWARE_PATH)) {
  console.log(`Loading firmware from ${FIRMWARE_PATH}`);
  loadUF2(FIRMWARE_PATH, mcu);
  console.log('Firmware loaded successfully');
} else {
  console.error(`Firmware not found at ${FIRMWARE_PATH}`);
  process.exit(1);
}

console.log("USB CDC output is disabled, use RP2 stdio via UART");
/*
// wiring up RP2 USB CDC
const cdc = new USBCDC(mcu.usbCtrl);
cdc.onDeviceConnected = () => {
  console.log("USB CDC connected");
};
cdc.onSerialData = (value) => {
  process.stdout.write(value);
};
if (process.stdin.isTTY) {
  process.stdin.setRawMode(true);
}
process.stdin.on('data', (chunk) => {
  // 24 is Ctrl+X
  if (chunk[0] === 24) {
    process.exit(0);
  }
  for (const byte of chunk) {
    cdc.sendSerialByte(byte);
  }
});
*/

// Set up UART output
mcu.uart[0].onByte = (value) => {
  process.stdout.write(new Uint8Array([value]));
};
if (process.stdin.isTTY) {
  process.stdin.setRawMode(true);
}
process.stdin.on('data', (chunk) => {
  // 24 is Ctrl+X, 3 is Ctrl-C
  if (chunk[0] === 24 || chunk[0] === 3) {
    process.exit(0);
  }
  for (const byte of chunk) {
    // console.log(`key in: ${byte}`);
    mcu.uart[0].feedByte(byte);
  }
});

// Set initial program counter for both cores
mcu.core0.PC = INITIAL_PC;
mcu.core1.PC = INITIAL_PC;

// GPIO state tracking
// Start with the initial CPU pin state
let currentPins = initialCpuPins;

mcu.gpio[29].setInputValue(1); // start with RESET high/inactive

// Main emulation loop
console.log('Starting emulation...');
console.log('Press Ctrl+C/Ctrl+X to stop');

let nextTimeUpdate = 0;
let prevClockState = false;
let cpuCycleCount = 0;

// original 65C02 bus timing (at 5V):
// CPU reads data on falling PHI2 (expects it set up tDSR=10ns before PHI2 falling and held tDHR=10ns after PHI2 falling)
// CPU writes data tMDS=25ns after rising PHI2, holds it until tDHW=10ns after falling PHI2
// CPU writes address/RW/SYNC tADS=30ns after falling PHI2, holds it until tAH=10ns after falling PHI2
// IRQ,NMI,RDY,RES is tPCS/tPCH, just like data read
// in short: fall phi2 - CPU reads data/flags - 30ns - CPU writes address - rise phi2 - 25ns - CPU writes data

let cpuTickCountdown = -1;
const cpuPosedgeToCPUDataWrite = Math.round(25*RP_MHZ/1000);
const cpuNegedgeToCPUAddrWrite = Math.round(30*RP_MHZ/1000);

function runEmulation() {
  for (let j = 0; j < 200000; j++) {
    // Step the RP2040
    let rpStartCycles = mcu.cycles;
    mcu.step();
    let rpCyclesElapsed = mcu.cycles - rpStartCycles;

    const currentClockState = mcu.gpio[25].value;
    if (currentClockState && !prevClockState) {
      // positive clock edge: after 25ns, write data
      cpuTickCountdown = cpuPosedgeToCPUDataWrite;
    } else if (!currentClockState && prevClockState) {
      // negative clock edge: read data/flags; then, after 30ns, write next address
      cpuTickCountdown = cpuNegedgeToCPUAddrWrite;

      if (currentPins & MASK_RW) {
        // Get RP2040 data (GPIO 16-23) and put on 65c02 pins
        for (let i = 16; i < 24; i++) {
          if (mcu.gpio[i].value == GPIOPinState.High) {
            currentPins |= (1n << BigInt(i));
          } else {
            currentPins &= ~(1n << BigInt(i));
          }
        }
      }
      // Copy RP2040 control signals to 6502 pins
      // Note: IRQ, NMI, RESET, RDY are active-low on GPIO but active-high in m6502
      if (!mcu.gpio[26].value) currentPins |= MASK_RDY; else currentPins &= ~MASK_RDY;  // Inverted!
      if (!mcu.gpio[27].value) currentPins |= MASK_IRQ; else currentPins &= ~MASK_IRQ;  // Inverted!
      if (!mcu.gpio[28].value) currentPins |= MASK_NMI; else currentPins &= ~MASK_NMI;  // Inverted!
      if (!mcu.gpio[29].value) { currentPins |= MASK_RESET; currentPins |= MASK_SYNC; } else currentPins &= ~MASK_RESET;  // Inverted, also set SYNC if we have RESET to get m6502 unstuck
    }
    prevClockState = currentClockState;

    if (cpuTickCountdown > 0) {
      cpuTickCountdown -= rpCyclesElapsed;
      if (cpuTickCountdown < 0) {
        cpuTickCountdown = 0;
      }
    }

    if (cpuTickCountdown == 0 && currentClockState == GPIOPinState.High) {
      // 25ns after PHI2 positive edge: write data (if CPU signals write)
      cpuTickCountdown--;

      if (!(currentPins & MASK_RW)) {
        // Copy m6502 bus data to RP2040 GPIO 16-23
        for (let i = 16; i < 24; i++) {
          const value = (currentPins & (1n << BigInt(i))) !== 0n;
          mcu.gpio[i].setInputValue(value);
        }
      }

    } else if (cpuTickCountdown == 0 && currentClockState == GPIOPinState.Low) {
      // 30ns after PHI2 negative edge: clock m65c02
      // internally, consumes data (on read cycle) or writes next data (on write cycle)
      // externally, writes next bus address
      cpuTickCountdown--;

      if (DEBUG) {
        const addr = Number(currentPins & MASK_ADDRESS);
        const data = Number((currentPins & MASK_DATA) >> BigInt(SHIFT_DATA));
        const rw = (currentPins & MASK_RW) !== 0n ? 'R' : 'W';
        const clock = (currentPins & MASK_CLOCK) !== 0n ? '1' : '0';
        const rdy = (currentPins & MASK_RDY) !== 0n ? '1' : '0';
        const irq = (currentPins & MASK_IRQ) !== 0n ? '1' : '0';
        const nmi = (currentPins & MASK_NMI) !== 0n ? '1' : '0';
        const reset = (currentPins & MASK_RESET) !== 0n ? '1' : '0';
        const sync = (currentPins & MASK_SYNC) !== 0n ? '1' : '0';
        console.log(`CPU #${cpuCycleCount.toString().padStart(6, ' ')}: ` +
                    `BusAddr=0x${addr.toString(16).padStart(4, '0')} ` +
                    `BusData=0x${data.toString(16).padStart(2, '0')} ` +
                    `RW=${rw} CLK=${clock} RDY=${rdy} IRQ=${irq} NMI=${nmi} RST=${reset} SYNC=${sync} `);
      }

      // Tick the m6502 CPU
      currentPins = BigInt(cpu_card_tick(Number(currentPins)));
      cpuCycleCount++;

      // Copy m6502 CPU address lines to RP2040 GPIO 0-15
      for (let i = 0; i < 16; i++) {
        const value = (currentPins & (1n << BigInt(i))) !== 0n;
        mcu.gpio[i].setInputValue(value);
      }

      // Update CPU output control signals
      mcu.gpio[24].setInputValue((currentPins & MASK_RW) !== 0n);
    }
    mcu.gpio[25].setInputValue(mcu.gpio[25].value); // just read back CLOCK value
    mcu.gpio[26].setInputValue(mcu.gpio[26].value); // just read back RDY value
    mcu.gpio[27].setInputValue(mcu.gpio[27].value); // just read back IRQ value
    mcu.gpio[28].setInputValue(mcu.gpio[28].value); // just read back NMI value
    mcu.gpio[29].setInputValue(mcu.gpio[29].value); // just read back RESET value

    if (mcu.cycles > nextTimeUpdate) {
      const time = ((mcu.cycles / 125000000) >>> 0) / 10;
      console.log(`\nTime: ${time.toFixed(1)}s, rpCycleCount: ${mcu.cycles}, 65C02 Cycles: ${cpuCycleCount}\n`);
      nextTimeUpdate += 40000000;
    }
  }
  if (MAX_CYCLES < 0 || mcu.cycles < MAX_CYCLES) {
    setTimeout(runEmulation);
  } else {
    console.log(`\nEmulation stopped after ${mcu.cycles} rp cycles and ${cpuCycleCount} 65C02 cycles`);
  }
}

// have to use setTimeout as otherwise the keyboard callback never gets called
setTimeout(runEmulation);
