
@ =========================================================================
@ Copyright (c) 2026 Sven Oliver "SvOlli" Moll
@
@ SPDX-License-Identifier: GPL-3.0-or-later
@
@ This program implements a JAM (Just Another Machine) custom platform
@ for the Sorbus Computer
@ =========================================================================
@ This file implements the interrupt handler invoked by pushing a value in
@ the multicore fifo from the bus core
@ =========================================================================

#include <hardware/regs/addressmap.h>
#include <hardware/regs/sio.h>
#include <hardware/regs/timer.h>
#include <hardware/regs/io_bank0.h>
#include <hardware/regs/pads_bank0.h>
#include <hardware/regs/m0plus.h>

@ =========================================================================
@ Notes that will help with implementation
@ =========================================================================
@ - at 133MHz, each RP2040 clock cycle takes 7.518796992 ns
@ - ideally 32 cycles in each clock high and low state will result in a
@   2MHz clock for the 65c02
@ =========================================================================

@ =========================================================================
@ Set up
@ =========================================================================
.syntax unified
.cpu cortex-m0plus
.thumb

@ =========================================================================
@ External C variables
@ =========================================================================
.extern cycle_counter

@ =========================================================================
@ Assembly code section
@ =========================================================================

.section .text.bus_loop_asm, "ax"
.align 2

.global bus_loop_asm
.type bus_loop_asm, %function

bus_loop_asm:
   @ clear cycle counter
   movs  r7, #0
   mov   r8, r7
   @ set constant: start of external I/O
   ldr   r7, =0xD000
   mov   r10, r7
   @ set constant: start of internal I/O
   ldr   r7, =0xDF00
   mov   r11, r7
   @ set constant: start of ROM
   ldr   r7, =0xE000
   mov   r12, r7
   @ from here on r7 is always SIO_BASE
   ldr   r7, =SIO_BASE
   @ since the low clock of 6502 will be used for aftermath, start with high
   b     .Lhighclock_6502

.Lloopclock:
   @ just entered low phase here
   @ move bits up to discard bits and get them down again
   @ assumption: r0 contains bus data
   @ assumption: r8 contains cycle counter
   movs  r1, #1                              @ 1cy
   add   r8, r1                              @ 1cy
   mov   r6, r8                              @ 1cy
   ldr   r5, =cycle_counter                  @ 2cy
   str   r5, [r6]                            @ 2cy
   lsls  r5, r5, #20                         @ 1cy
   lsrs  r5, r5, #20                         @ 1cy
   ldr   r6, =bus_log                        @ 2cy
   str   r0, [r6, r5]                        @ 2cy

.Lhighclock_6502:
   ldr   r6, =0x02000000                     @ 2cy
   str   r6, [r7, #SIO_GPIO_OUT_CLR_OFFSET]  @ 2cy

   @ make sure data bus is on input
   ldr   r5, =0x00FF0000                     @ 2cy
   str   r5, [r7, #SIO_GPIO_OE_CLR_OFFSET]   @ 2cy
   @ get GPIOs (must not occur before 140ns after clock high)
   ldr   r0, [r7, #SIO_GPIO_IN_OFFSET]       @ 2cy

   @ get correct bank pointer
   ldr   r4, =romptr                         @ 2cy
   lsls  r3, r0, #7  @now r/w is at negative @ 1cy
   bpl   .Lforceram  @write always to RAM    @ 1/2cy
   @ check for
   cmp   r4, r12     @$E000                  @ 1cy
   bcs   .Lnoram                             @ 1/2cy
.Lforceram:
   ldr   r4, =ramptr                         @ 2cy
.Lnoram:
   @now r4 point to correct position in SRAM

   @ check address
   @ r1: 
   uxth  r1, r0                              @ 1cy
   
   cmp   r1, #0x4
   bcc   .Lhandsoff
   cmp   r1, r10 @ 0xD000
   bcc   .Lram
   cmp   r1, r11 @ 0xDF00
   bcc   .Lhandsoff
   bne   .Lnobankswitch
   cmp   r1, r12 @ 0xE000
   bcc   .Lintio
.Lramrom:

.Lram:

.Lmem:

   

.Llowclock_6502:
   ldr   r6, =0x02000000
   str   r6, [r7, #SIO_GPIO_OUT_CLR_OFFSET]
   b     .Lloopclock

