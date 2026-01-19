#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CHIPS_IMPL
#include "m65c02.h"

// Global CPU state
static m65c02_t cpu;
static uint64_t pins;

// Initialize the CPU card
void cpu_card_init() {
    pins = m65c02_init(&cpu, &(m65c02_desc_t){});
}

// Get current pins state (for initialization)
uint64_t cpu_card_get_pins() {
    return pins;
}

// Tick the CPU for one cycle
// Input: pins value (uint64_t)
// Returns: updated pins value
uint64_t cpu_card_tick(uint64_t input_pins) {
    pins = input_pins;
    // Run the CPU emulation for one tick
    pins = m65c02_tick(&cpu, pins);
    return pins;
}
