
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

.section .time_critical.my_irq_handler, "ax", %progbits
.align 2

@ =========================================================================
@ External C variables
@ =========================================================================
.extern cycle_counter
.extern bankptrs
.extern ram
.extern romptr

@ =========================================================================
@ Register usage
@ =========================================================================

@  r0: 65c02 cpu bus as read by GPIOs
@  r1: 65c02 addressbus
@  r2: 65c02 databus
@  r3: #0xD0000000: RP2040 SIO bank start
@  r4: #0x02000000: 65C02 R/W mask
@  r5: 
@  r6: 
@  r7: 
@  r8: cycle_counter
@  r9: 
@ r10: #0x0000D000: 65C02 I/O start
@ r11: #0x0000DF00: 65C02 internal I/O start
@ r12: #0x0000E000: 65C02 ROM start
@ r13/sp:
@ r14/lr:
@ r15/pc:

@ =========================================================================
@ Assembly code section
@ =========================================================================

.section .text.bus_loop_asm, "ax"
.align 2

.global bus_loop_asm
.type bus_loop_asm, %function

bus_loop_asm:
   @ clear cycle counter
   movs  r3, #0
   mov   r8, r3
   @ set constant: start of external I/O
   ldr   r3, =0xD000
   mov   r10, r3
   @ set constant: start of internal I/O
   ldr   r3, =0xDF00
   mov   r11, r3
   @ set constant: start of ROM
   ldr   r3, =0xE000
   mov   r12, r3
   @ from here on r3 is always SIO_BASE
   ldr   r3, =SIO_BASE
   ldr   r4, =0x02000000
   @ since the low clock of 6502 will be used for aftermath, start with high
   b     .Lhighclock_6502


@ -------------------------------------------------------------------------
@ Steps to go
@ -------------------------------------------------------------------------
@ > set clock low
@ > set clock high
@ > loop
@ Cycle counter and logging
@ Prepare high phase:
@ - set data bus to read
@ - read GPIOs
@ - calculate address
@ RAM / ROM / I/O access check
@ Set bank
@ Get from RAM/ROM to data bus
@ Send GPIO state via FIFO


@ =========================================================================
@ Routine catalog
@ =========================================================================


@ -------------------------------------------------------------------------
@ Cycle counter and logging
@ -------------------------------------------------------------------------
@ assumption: r0 contains bus data
@ assumption: r4 contains clock mask
@ assumption: r8 contains cycle counter
@ using: r5
@ using: r6
@ cycles: 4, 17

   @ CLK is pin 26, RDY is pin 27
   lsls  r6, r4, #1                          @ 1cy
   ands  r6, r0                              @ 1cy
   beq   .Lnocount                           @ 1cy --> 4cy

   movs  r6, #1                              @ 1cy
   @ high register form
   add   r8, r6                              @ 1cy
   mov   r6, r8                              @ 1cy
   @ store cycle counter for C
   ldr   r5, =cycle_counter                  @ 2cy
   str   r6, [r5]                            @ 2cy
   @ move bits up to discard bits and get them down again
   @ --> and #0x00000FFF
   lsls  r6, r6, #20                         @ 1cy
   lsrs  r6, r6, #20                         @ 1cy
   @ align index to 32bit addressing
   lsls  r6, r6, #2                          @ 1cy
   ldr   r5, =bus_log                        @ 2cy
   str   r0, [r5, r6]                        @ 2cy
.Lnocount:


@ -------------------------------------------------------------------------
@ Prepare high phase:
@ - set data bus to read
@ - read GPIOs
@ - calculate address
@ -------------------------------------------------------------------------
@ assumption: r3 contains SIO_BASE
@ using: r0 -> bus data
@ cycles: 5 (3, if #0x00FF0000 gets its own register)

   @ a branch for the loop is required
   @ put it in front of this after the clock high?
   @ would add up to 6 cycles (see below)

   movs  r0, #0xFF                           @ 1cy
   lsls  r0, r0, #16                         @ 1cy
   @ r0 = 0x00FF0000
   str   r0, [r3, #SIO_GPIO_OUT_CLR_OFFSET]  @ 1cy
   @ get GPIOs (must not occur before 40ns (6cy) after clock high,
   @            then data bus write will be also read)
   ldr   r0, [r3, #SIO_GPIO_IN_OFFSET]       @ 1cy
   @ also prepare address word
   uxth  r1, r0                              @ 1cy


@ -------------------------------------------------------------------------
@ RAM / ROM / I/O access check
@ -------------------------------------------------------------------------

   lsrs  r7, r4, #1                          @ 1cy
   ands  r7, r0                              @ 1cy
   bne   .Lmemread                           @ 1cy --> 4cy
                                             @ --> 3cy

   @ selecting function according to writing
   @ address < 0x0004 can be written, will just never be read
   @ writing >= 0xE000 always writes to RAM, so extra check after 0xDF80
   @ 0xDF00 triggers banking
   @ 0xDF00-0xDF7F triggers FIFO (and is RAM)
   @ 0xD000-0xDEFF must be hands off
   uxtb  r2, r0                              @ 1cy
   lsls  r2, r2, #16                         @ 1cy

   cmp   r1, r10 @ #0xD000                   @ 1cy
   bcc   .L0ramwrite                         @ 1cy --> 5cy
   cmp   r1, r11 @ #0xDF00                   @ 1cy
   bcc   .L0handoff                          @ 1cy --> 7cy
   bne   .L0nobankswitch                     @ 1cy --> 8cy
                                             @ --> 7cy

.L0nobankswitch:
   uxtb  r7, r0                              @ 1cy
   cmp   r7, #0x80                           @ 1cy
   bcs   .L0ramwrite                         @ 1cy --> 4cy
                                             @ 3cy
.L0fifosend:
   @ TODO: inline here                         7cy

.L0ramwrite:
   ldr   r6, =ram                            @ 2cy
   str   r2, [r6, r1]                        @ 2cy
   @ TODO: where next?


.Lmemread:
   @ selecting function according to reading
   @ 0x0000-0x0003 and 0xD000-0xDEFF must be hands off
   @ 0xDF00-0xDF0F triggers FIFO (and is RAM)
   @ >= 0xE000 is ROM
   @ rest is RAM

   @ get data pointer: RAM or ROM
   ldr   r7, =romvec                         @ 2cy
   cmp   r1, r12 @ #0xE000                   @ 1cy
   bcs   .Lromread                           @ 1cy --> 5cy
   ldr   r7, =ram                            @ 2cy
                                             @ --> 6cy

   @ check hands off
   cmp   r1, #0x0004                         @ 1cy
   bcc   .Lhandsoff                          @ 1cy --> 3cy
   cmp   r1, r10 @ #0xD000                   @ 1cy
   bcc   .Ldatatobus                         @ 1cy --> 5cy
   cmp   r1, r11 @ #0xDF00                   @ 1cy
   bcc   .Lhandsoff                          @ 1cy --> 7cy
                                             @ --> 6cy

.Ldatatobus:
   @ get the data from SRAM to the bus
   ldrb  r2, [r7, r1]                        @ 1cy
   lsls  r2, r2, #16                         @ 1cy
   movs  r6, #0xFF                           @ 1cy
   lsls  r6, r6, #16                         @ 1cy
   bics  r0, r0, r6   @ r7 = r0 & ~r6        @ 1cy
   orrs  r0, r0, r2                          @ 1cy
   str   r0, [r3, #SIO_GPIO_OUT_OFFSET]      @ 2cy
                                             @ --> 8cy (6cy @ 0x00FF0000)

   @ now check for read FIFO
   lsrs  r7, r1, #8                          @ 1cy
   cmp   r7, #0xDF                           @ 1cy
   bne   .Lnofifoonread                      @ 1cy --> 4cy
   uxtb  r7, r1                              @ 1cy
   cmp   r7, #0x10                           @ 1cy
   bcs   .Lnofifoonread                      @ 1cy --> 7cy
                                             @ --> 6cy

   @ put GPIOs in FIFO
   ldr   r7, =SIO_FIFO_ST_RDY_BITS           @ 2cy
   ldr   r6, [r3, #SIO_FIFO_ST_OFFSET]       @ 1cy
   ands  r7, r6                              @ 1cy
   beq   .Lskipsend                          @ 1cy --> 6cy
   str   r0, [r3, #SIO_FIFO_WR_OFFSET]       @ 2cy
                                             @ --> 7cy

.Lnofifoonread:


.Lhandsoff:


@ try 1: high to low
   cmp   r1, #4                              @ 1cy
   bcc   .L1handsoff                         @ 1cy --> 3cy
   cmp   r1, r10 @ 0xD000                    @ 1cy
   bcc   .L1ram                              @ 1cy --> 5cy
   cmp   r1, r11 @ 0xDF00                    @ 1cy
   beq   .L1bank                             @ 1cy --> 7cy
   bcc   .L1handsoff                         @ 1cy --> 8cy
   cmp   r1, r12 @ 0xE000                    @ 1cy
   bcc   .L1intio                            @ 1cy --> 10cy
.L1ram:

.L1rom:

.L1handsoff:

.L1bank:


@ try 2: high to low
   cmp   r1, r12 @ 0xE000                    @ 1cy
   bcs   .L2rom                              @ 1cy --> 3cy
   cmp   r1, r11 @ 0xDF00                    @ 1cy
   beq   .L2bank                             @ 1cy --> 5cy
   bcs   .L2intio                            @ 1cy --> 6cy
   cmp   r1, r10 @ 0xD000                    @ 1cy
   bcs   .L2handsoff                         @ 1cy --> 8cy
   cmp   r1, #4                              @ 1cy
   bcc   .L2handsoff                         @ 1cy --> 10cy
.L2ram:
   ldr   r7, =ram
   
.L2rom:
   ldr   r7, =romptr

.L2handsoff:

.L2bank:


@ -------------------------------------------------------------------------
@ Set bank
@ -------------------------------------------------------------------------
@ assumption: r0 contains GPIO data
@ using r6, r7

   lsrs  r7, r0, #16                         @ 1cy
   uxtb  r7, r7                              @ 1cy
   @ align index to 32bit addressing
   lsls  r6, r7, #2                          @ 1cy
   ldr   r7, =bankptrs                       @ 2cy
   ldr   r7, [r7, r6]                        @ 2cy
   ldr   r6, =romptr                         @ 2cy
   str   r7, [r6]                            @ 2cy
                                             @ --> 11cy


@ -------------------------------------------------------------------------
@ Get from RAM/ROM to data bus
@ -------------------------------------------------------------------------
@ assumption: r1 contains address bus
@ assumption: r3 contains SIO_BASE
@ assumption: r7 contains memory pointer (RAM/ROM)
@ using r6

   movs  r6, #0xFF                           @ 1cy
   lsls  r6, r6, #16                         @ 1cy
   @ r6 = 0x00FF0000
   @ set data direction to output
   str   r6, [r3, #SIO_GPIO_OUT_SET_OFFSET]  @ 1cy
   ldrb  r6, [r1, r7]                        @ 1cy
   lsls  r6, r6, #16                         @ 1cy
   @ write data
   str   r6, [r3, #SIO_GPIO_OUT_OFFSET]      @ 1cy


@ -------------------------------------------------------------------------
@ Send GPIO state via FIFO
@ -------------------------------------------------------------------------
@ assumption: r0 contains GPIO data
@ assumption: r3 contains SIO_BASE

   ldr   r7, =SIO_FIFO_ST_RDY_BITS           @ 2cy
   ldr   r6, [r3, #SIO_FIFO_ST_OFFSET]       @ 1cy
   ands  r7, r6                              @ 1cy
   beq   .Lskipsend                          @ 1cy --> 6cy
   str   r0, [r3, #SIO_FIFO_WR_OFFSET]       @ 2cy
                                             @ --> 7cy
.Lskipsend:



@ =========================================================================
@ Conceptional stuff
@ =========================================================================


.Lhighclock_6502:
   ldr   r6, =0x02000000                     @ 2cy
   str   r6, [r3, #SIO_GPIO_OUT_CLR_OFFSET]  @ 1cy

   @ get correct bank pointer
   ldr   r4, =romptr                         @ 2cy
   lsls  r6, r0, #7  @now r/w is at negative @ 1cy
   bpl   .Lforceram  @write always to RAM    @ 1/2cy
   @ check for
   cmp   r4, r12     @$E000                  @ 1cy
   bcs   .Lnoram                             @ 1/2cy
.Lforceram:
   ldr   r4, =ramptr                         @ 2cy
.Lnoram:
   @now r4 point to correct position in SRAM

.Lcheck:
   @ check address
   @ r1: 
   uxth  r1, r0                              @ 1cy
   lsrs  r2, r0, #16                         @ 1cy
   uxtb  r2, r2                              @ 1cy

   @ check for zero page I/O (VGA)
   cmp   r1, #0x0004                         @ 1cy
   bcc   .Lhandsoff                          @ 1/2cy --> 6cy
   cmp   r1, r10 @ 0xD000                    @ 1cy
   bcc   .Lram                               @ 1/2cy --> 8cy
   cmp   r1, r11 @ 0xDF00                    @ 1cy
   beq   .Lsetbank                           @ 1/2cy --> 10cy
   bcc   .Lhandsoff                          @ 1/2cy --> 11cy
   cmp   r1, r12 @ 0xE000                    @ 1cy
   bcc   .Lintio                             @ 1/2cy --> 13cy
.Lramrom:

.Lram:

.Lmem:


.Lsetbank:
   @ set bank offset for romptr
   @
   @ assumption: r2 contains 8bit data bus
   @ using r6, r3
   @ 9 cycles
   lsls  r6, r2, #2           @ 1 cycle
   ldr   r3, =bankptrs        @ 2 cycles
   ldr   r3, [r3, r6]         @ 2 cycles
   ldr   r6, =romptr          @ 2 cycles
   str   r3, [r6]             @ 2 cycles

.Lramrom2:
   @ assumption: r1 contains 16bit address bus
   @ assumption: r7 contains SIO
   @ assumption: r12 contains constant 0xE000
   @ 11 cycles
   cmp   r1, r12  @ #0xE000   @ 1cy
   bcs   read_rom             @ 1/2cy
   ldr   r6, =ram             @ 2cy
   b     read_all             @ 2cy
read_rom:
   nop                        @ 1cy
   ldr   r6, =romptr          @ 2cy
read_all:
   ldrb  r7, [r6, r1]         @ 2cy
   lsls  r7, r0, #16          @ 1cy
   str   r7, [r3, #SIO_GPIO_OUT_OFFSET] @ 2cy



.Lsleep6:
   nop
.Lsleep5:
   bx    lr
