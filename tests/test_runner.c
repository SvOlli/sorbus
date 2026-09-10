// Sorbus Testing Framework
//
// This test framework runs the RP2040 chipset code using the cts2c-transpiled
// rp2350js emulator (rp2350js-c.h) and wires it up with the 65C02 CPU card
// emulation (m65c02.h). It replaces the former JavaScript test_runner.js +
// koffi FFI bridge.
//
// USB CDC console: the jam firmware uses stdio_usb, so the runner instantiates
// the emulator-side USBCDC host and bridges it to stdin/stdout.
//
// GPIO Pin Mapping (matching bus.h):
// - Bits 0..15:   Address (A0-A15)
// - Bits 16..23:  Data (D0-D7)
// - Bit 24:       RW line (low: write)
// - Bit 25:       Clock line
// - Bit 26:       RDY line (CPU halts on low)
// - Bit 27:       IRQ line (active low)
// - Bit 28:       NMI line (active low)
// - Bit 29:       Reset line (active low)

#define _DEFAULT_SOURCE // poll(), termios raw mode
#include "rp2350js-c.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_CYCLES (-1)
#define DEBUG 0
#define RP_MHZ 125

#define FIRMWARE_PATH "../build/rp2040/jam_alpha_picotool.uf2"
#define INITIAL_PC 0x10000000u

// Pin masks (matching bus.h)
#define MASK_ADDRESS 0x00000000FFFFull // Bits 0-15
#define MASK_DATA 0x000000FF0000ull    // Bits 16-23
#define MASK_RW 0x000001000000ull      // Bit 24
#define MASK_CLOCK 0x000002000000ull   // Bit 25
#define MASK_RDY 0x000004000000ull     // Bit 26
#define MASK_IRQ 0x000008000000ull     // Bit 27
#define MASK_NMI 0x000010000000ull     // Bit 28
#define MASK_RESET 0x000020000000ull   // Bit 29
#define MASK_SYNC 0x000040000000ull    // Bit 30

// ─── 65C02 CPU card (m65c02 emulation, formerly cpu_card_wrapper.c) ─────
#define CHIPS_IMPL
#include "m65c02.h"

static m65c02_t cpu_card;

static RP2040* mcu;
static USBCDC__RP2040* cdc;
static struct termios saved_termios;
static bool termios_saved = false;

static uint64_t currentPins;
static int64_t cpuCycleCount = 0;

// ─── USB CDC  console ──────────────────────────────────────
static void console_output(uint8_t byte) {
  putchar(byte);
  fflush(stdout);
}

static void cdc_on_serial_data(void* ctx, uint8_t* buffer, int32_t length) {
  (void)ctx;
  fwrite(buffer, 1, length, stdout);
  fflush(stdout);
}

static void cdc_on_device_connected(void* ctx) {
  (void)ctx;
  printf("USB CDC connected. Keypress for Sorbus menu. CTRL-C to exit.\n");
  fflush(stdout);
}

// ─── stdin (raw mode, Ctrl+X exits) ─────────────────────────────────────
static void restore_termios(void) {
  if (termios_saved) tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
}

static void sigint_handler(int sig) {
  (void)sig;
  restore_termios();
  printf("\n");
  exit(0);
}

static void setup_stdin(void) {
  struct termios t;
  if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &t) == 0) {
    saved_termios = t;
    termios_saved = true;
    cfmakeraw(&t);
    // keep output post-processing (ONLCR): without it every '\n' from
    // BasePeripheral_debug() & co renders as a bare LF (staircase effect)
    t.c_oflag |= OPOST;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
  }
  atexit(restore_termios);
  signal(SIGINT, sigint_handler);
}

static bool stdin_eof = false;

static void poll_stdin(void) {
  struct pollfd pfd = {.fd = STDIN_FILENO, .events = POLLIN};
  if (stdin_eof || poll(&pfd, 1, 0) <= 0) return;
  uint8_t chunk[256];
  ssize_t n = read(STDIN_FILENO, chunk, sizeof chunk);
  if (n == 0) { stdin_eof = true; return; } // EOF: stop polling, keep emulating
  for (ssize_t k = 0; k < n; k++) {
    // 3 is Ctrl+C
    if (chunk[k] == 3) {
      restore_termios();
      printf("\n");
      exit(0);
    }
    USBCDC__RP2040_sendSerialByte(cdc, chunk[k]);
  }
}

// ─── main ───────────────────────────────────────────────────────────────
int main(void) {
  printf("Initializing 65C02 CPU card...\n");
  currentPins = m65c02_init(&cpu_card, &(m65c02_desc_t){});
  printf("Initial CPU pins: 0x%llx\n", (unsigned long long)currentPins);

  printf("Initializing RP2040...\n");
  FILE* fw = fopen(FIRMWARE_PATH, "rb");
  if (!fw) {
    fprintf(stderr, "Firmware not found at %s\n", FIRMWARE_PATH);
    return 1;
  }
  fclose(fw);
  printf("Loading firmware from %s\n", FIRMWARE_PATH);
  RP2040Options options = {.loadFirmware = FIRMWARE_PATH};
  mcu = RP2040_new(&options);
  printf("Firmware loaded successfully\n");

  // wiring up RP2 USB CDC
  cdc = USBCDC__RP2040_new(mcu->usbCtrl);
  cdc->onSerialData_fn = cdc_on_serial_data;
  cdc->onSerialData_ctx = NULL;
  cdc->onDeviceConnected_fn = cdc_on_device_connected;
  cdc->onDeviceConnected_ctx = NULL;

  GPIOPin__RP2040_setInputValue(mcu->gpio[29], true); // start with RESET high/inactive

  setup_stdin();

  bool prevClockState = false;

  // original 65C02 bus timing (at 5V):
  // CPU reads data on falling PHI2 (expects it set up tDSR=10ns before PHI2 falling and held tDHR=10ns after PHI2 falling)
  // CPU writes data tMDS=25ns after rising PHI2, holds it until tDHW=10ns after falling PHI2
  // CPU writes address/RW/SYNC tADS=30ns after falling PHI2, holds it until tAH=10ns after falling PHI2
  // IRQ,NMI,RDY,RES is tPCS/tPCH, just like data read
  // in short: fall phi2 - CPU reads data/flags - 30ns - CPU writes address - rise phi2 - 25ns - CPU writes data

  int64_t cpuTickCountdown = -1;
  int64_t stdinPollCountdown = 0;
  const int64_t cpuPosedgeToCPUDataWrite = (25 * RP_MHZ + 500) / 1000; // round(25ns at RP_MHZ)
  const int64_t cpuNegedgeToCPUAddrWrite = (30 * RP_MHZ + 500) / 1000; // round(30ns at RP_MHZ)

  for (;;) {
    // Step the RP2040
    int64_t rpStartCycles = RP2040_cycles_get(mcu);
    RP2040_step(mcu);
    int64_t rpCyclesElapsed = RP2040_cycles_get(mcu) - rpStartCycles;

    int32_t currentClockState = GPIOPin__RP2040_value_get(mcu->gpio[25]);
    if (currentClockState == GPIOPinState_High && !prevClockState) {
      // positive clock edge: after 25ns, write data
      cpuTickCountdown = cpuPosedgeToCPUDataWrite;
    } else if (currentClockState == GPIOPinState_Low && prevClockState) {
      // negative clock edge: read data/flags; then, after 30ns, write next address
      cpuTickCountdown = cpuNegedgeToCPUAddrWrite;

      if (currentPins & MASK_RW) {
        // Get RP2040 data (GPIO 16-23) and put on 65c02 pins
        for (int i = 16; i < 24; i++) {
          if (GPIOPin__RP2040_value_get(mcu->gpio[i]) == GPIOPinState_High)
            currentPins |= (1ull << i);
          else
            currentPins &= ~(1ull << i);
        }
      }
      // Copy RP2040 control signals to 6502 pins
      // Note: IRQ, NMI, RESET, RDY are active-low on GPIO but active-high in m65c02
      if (GPIOPin__RP2040_value_get(mcu->gpio[26]) != GPIOPinState_High) currentPins |= MASK_RDY;
      else currentPins &= ~MASK_RDY;                                                    // Inverted!
      if (GPIOPin__RP2040_value_get(mcu->gpio[27]) != GPIOPinState_High) currentPins |= MASK_IRQ;
      else currentPins &= ~MASK_IRQ;                                                    // Inverted!
      if (GPIOPin__RP2040_value_get(mcu->gpio[28]) != GPIOPinState_High) currentPins |= MASK_NMI;
      else currentPins &= ~MASK_NMI;                                                    // Inverted!
      if (GPIOPin__RP2040_value_get(mcu->gpio[29]) != GPIOPinState_High) {
        currentPins |= MASK_RESET;
        currentPins |= MASK_SYNC; // also set SYNC if we have RESET to get m65c02 unstuck
      } else {
        currentPins &= ~MASK_RESET; // Inverted
      }
    }
    prevClockState = currentClockState == GPIOPinState_High;

    if (cpuTickCountdown > 0) {
      cpuTickCountdown -= rpCyclesElapsed;
      if (cpuTickCountdown < 0) {
        cpuTickCountdown = 0;
      }
    }

    if (cpuTickCountdown == 0 && currentClockState == GPIOPinState_High) {
      // 25ns after PHI2 positive edge: write data (if CPU signals write)
      cpuTickCountdown--;

      if (!(currentPins & MASK_RW)) {
        // Copy m65c02 bus data to RP2040 GPIO 16-23
        for (int i = 16; i < 24; i++) {
          GPIOPin__RP2040_setInputValue(mcu->gpio[i], (currentPins & (1ull << i)) != 0);
        }
      }

    } else if (cpuTickCountdown == 0 && currentClockState == GPIOPinState_Low) {
      // 30ns after PHI2 negative edge: clock m65c02
      // internally, consumes data (on read cycle) or writes next data (on write cycle)
      // externally, writes next bus address
      cpuTickCountdown--;

      if (DEBUG) {
        printf("CPU #%06ld: BusAddr=0x%04llx BusData=0x%02llx RW=%c CLK=%d RDY=%d IRQ=%d NMI=%d RST=%d SYNC=%d \n",
               (long)cpuCycleCount,
               (unsigned long long)(currentPins & MASK_ADDRESS),
               (unsigned long long)((currentPins & MASK_DATA) >> 16),
               (currentPins & MASK_RW) ? 'R' : 'W',
               (currentPins & MASK_CLOCK) ? 1 : 0,
               (currentPins & MASK_RDY) ? 1 : 0,
               (currentPins & MASK_IRQ) ? 1 : 0,
               (currentPins & MASK_NMI) ? 1 : 0,
               (currentPins & MASK_RESET) ? 1 : 0,
               (currentPins & MASK_SYNC) ? 1 : 0);
      }

      // Tick the m65c02 CPU
      currentPins = m65c02_tick(&cpu_card, currentPins);
      cpuCycleCount++;

      // Copy m65c02 CPU address lines to RP2040 GPIO 0-15
      for (int i = 0; i < 16; i++) {
        GPIOPin__RP2040_setInputValue(mcu->gpio[i], (currentPins & (1ull << i)) != 0);
      }

      // Update CPU output control signals
      GPIOPin__RP2040_setInputValue(mcu->gpio[24], (currentPins & MASK_RW) != 0);
    }
    // just read back CLOCK/RDY/IRQ/NMI/RESET values
    for (int i = 25; i < 30; i++) {
      GPIOPin__RP2040_setInputValue(mcu->gpio[i],
                                    GPIOPin__RP2040_value_get(mcu->gpio[i]) == GPIOPinState_High);
    }

    stdinPollCountdown -= rpCyclesElapsed;
    if (stdinPollCountdown <= 0) {
      stdinPollCountdown = 50000;
      poll_stdin();
    }

    int64_t cycles = RP2040_cycles_get(mcu);

    if (MAX_CYCLES >= 0 && cycles >= MAX_CYCLES) {
      printf("\nEmulation stopped after %lld rp cycles and %lld 65C02 cycles\n",
             (long long)cycles, (long long)cpuCycleCount);
      break;
    }
  }
  return 0;
}
