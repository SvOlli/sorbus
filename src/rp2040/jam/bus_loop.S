
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

#ifndef BUS_CONFIG_mask_clock
#define BUS_CONFIG_mask_clock 0x02000000
#endif

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

.section .time_critical.tight_loop, "ax", %progbits
.align 2

.global tight_loop
.type tight_loop, %function

@ =========================================================================
@ Macro Definitions
@ =========================================================================
.macro delay count
    .rept \count
        nop
    .endr
.endm

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
@  r4: #0x02000000: 65C02 CLK mask
@  r5: memptr
@  r6: scratch / end of fifo area / fifo flag
@  r7: scratch
@  r8: cycle_counter
@  r9: romptr
@ r10: #0x0000D000: 65C02 I/O start
@ r11: #0x0000DF00: 65C02 internal I/O start
@ r12: #0x0000E000: 65C02 ROM start
@ r13/sp:
@ r14/lr:
@ r15/pc:

@ =========================================================================
@ Assembly code section
@ =========================================================================

bus_loop_asm:
@ -------------------------------------------------------------------------
@ Setup
@ -------------------------------------------------------------------------
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
   @ from here on r4 is always 65C02 CLK
   ldr   r4, =BUS_CONFIG_mask_clock

@ --> 27cy
.Lmain_loop:
   @ r9 = romptr
   ldr   r5, =romptr                         @ 2cy
   mov   r9, r5                              @ 1cy

   @ r5 = memptr -> RAM
   ldr   r5, =ram                            @ 2cy

@ -------------------------------------------------------------------------
@ end of clock low phase
@ -------------------------------------------------------------------------

   @ r4 = 65C02 CLK, r3 = SIO_BASE
   str   r4, [r3, #SIO_GPIO_OUT_SET_OFFSET]  @ 1cy

@ -------------------------------------------------------------------------
@ Start of clock high phase
@ RP2040 cycle counting starts now
@ -------------------------------------------------------------------------

   @ r0 = 0x00FF0000
   movs  r0, #0xFF                           @ 1cy
   lsls  r0, r0, #16                         @ 1cy
   @ data bus direction: read
   str   r0, [r3, #SIO_GPIO_OE_SET_OFFSET]   @ 1cy

   @ CLK: pin 26 -> R/W: pin 25
   lsrs  r7, r4, #1                          @ 1cy

   @ r6: 0xDF00 (for later modification of end of internal I/O)
   mov   r6, r11                             @ 1cy

   @ get GPIOs (must not occur before 40ns (6cy) after clock high,
   @            then data bus write will be also read)
   ldr   r0, [r3, #SIO_GPIO_IN_OFFSET]       @ 1cy

   @ also prepare r1 = address word and r2 = data byte
   uxth  r1, r0                              @ 1cy
   lsrs  r2, r0, #16                         @ 1cy
   uxtb  r2, r2                              @ 1cy
                                             @ --> 9cy

   @ mask 65C02 R/W
   ands  r7, r0                              @ 1cy
   beq   .Lmemread                           @ 1cy --> 12cy
                                             @ --> 11cy

@ =========================================================================
@ handling memory write cases
@ =========================================================================
@ --> 11cy
   cmp   r1, r10  @ #0xD000                  @ 1cy
   bcc   .Lwrite_l_d000                      @ 1cy --> 14cy
   cmp   r1, r11  @ #0xDF00                  @ 1cy
   bne   .Lnobank                            @ 1cy --> 16cy

@ -------------------------------------------------------------------------
@ Set bank
@ -------------------------------------------------------------------------
@ assumption: r0 contains GPIO data
@ assumption: r2 contains data
@ using r6, r7
@ --> 15cy

   @ convert index to 32bit addressing
   lsls  r7, r2, #2                          @ 1cy

   @ get from list of pointers
   ldr   r6, =bankptrs                       @ 2cy
   ldr   r7, [r7, r6]                        @ 2cy

   @ write to variable
   ldr   r6, =romptr                         @ 2cy
   str   r7, [r6]                            @ 2cy
                                             @ --> 24cy

@ -------------------------------------------------------------------------
@ at the entry of high phase, r6 marks, if multicore fifo is used:
@ r6=0 use fifo, r6!=0 no fifo, unless explicitly cleared r6 is never 0
@ -------------------------------------------------------------------------
@ --> 24cy
.Lwritemarkfifo:
   movs  r6, #0                              @ 1cy
                                             @ --> 25cy

@ -------------------------------------------------------------------------
@ Write data to RAM
@ -------------------------------------------------------------------------
@ assumption: r1 contains address bus
@ assumption: r2 contains data
@ assumption: r5 contains memory pointer (RAM/ROM)
@ --> 25cy

.Lwritetoram:
   strb  r2, [r5, r1]                        @ 1cy
   b     .Lend_of_high_at_28cy                         @ 2cy --> 28cy

@ -------------------------------------------------------------------------
@ testing addresses >
@ -------------------------------------------------------------------------
@ --> 16cy
.Lnobank:
   @ coming in from cmp #0xDF00
   bcc   .Lwrite_l_df00                      @ 1cy --> 18cy
   delay 3                                   @ 3cy
   adds  r6, #0x80                           @ 1cy
   cmp   r1, r6   @ #0xDF80                  @ 1cy
   bcc   .Lwritemarkfifo                     @ 1cy --> 24cy
   b     .Lwritetoram                        @ 2cy --> 25cy

@ -------------------------------------------------------------------------
@ write is in $D000-$DEFF: do nothing
@ -------------------------------------------------------------------------
@ --> 18cy
.Lwrite_l_df00:
   delay 8                                   @ 8cy
   b     .Lend_of_high_at_28cy               @ 2cy --> 28cy

@ -------------------------------------------------------------------------
@ write is in $0000-$CFFF: write to ram after respecting timing
@ -------------------------------------------------------------------------
@ --> 14cy
.Lwrite_l_d000:
   delay 9                                   @ 9cy
   b     .Lwritetoram                        @ 2cy --> 25cy


@ =========================================================================
@ handling memory read cases
@ =========================================================================

@ -------------------------------------------------------------------------
@ comparisons besides the critical path
@ -------------------------------------------------------------------------
@ --> 15cy
.Lread_l_df00:
   cmp   r1, r10  @ #0xD000                  @ 1cy
   bcs   .Lread_g_d000                       @ 1cy --> 18cy
   cmp   r1, #0x0004                         @ 1cy
   bcs   .Ldataoutput                        @ 1cy --> 20cy
                                             @ --> 19cy
   b     .Lread_l_0004                       @ 2cy --> 21cy

@ --> 18cy
.Lread_g_d000:
   delay 3
@ --> 21cy
.Lread_l_0004:
   delay 5
   b     .Lend_of_high_at_28cy

.Lread_l_df10: @ 6cy --> 10cy --> .Ldataoutput
   movs  r6, #0x00   @ r6=0: trigger fifo    @ 1cy
   nop                                       @ 1cy
   b     .Ldataoutput                        @ 2cy


@ -------------------------------------------------------------------------
@ switch memory pointer to ROM bank pointer
@ -------------------------------------------------------------------------
@ --> 17cy
.Lread_g_e000:
   mov   r5, r9                              @ 1cy
   b     .Ldataoutput                        @ --> 20cy

@ -------------------------------------------------------------------------
@ read critical path
@ -------------------------------------------------------------------------
@ --> 12cy
.Lmemread:
   cmp   r1, r11  @ #0xDF00                  @ 1cy
   bcc   .Lread_l_df00                       @ 1cy --> 15cy

   cmp   r1, r12  @ #0xE000                  @ 1cy
   bcs   .Lread_g_e000                       @ 1cy --> 17cy

   adds  r6, #0x10                           @ 1cy
   cmp   r1, r6   @ #0xDF10                  @ 1cy
   bcs   .Ldataoutput                        @ 1cy --> 20cy
                                             @ --> 19cy

@ -------------------------------------------------------------------------
@ at the entry of high phase, r6 marks, if multicore fifo is used:
@ r6=0 use fifo, r6!=0 no fifo, unless explicitly cleared r6 is never 0
@ -------------------------------------------------------------------------
@ --> 19cy
   movs  r6, #0                              @ 1cy
                                             @ --> 20cy

.Ldataoutput:
@ -------------------------------------------------------------------------
@ get from RAM/ROM to data bus
@ -------------------------------------------------------------------------
@ assumption: r1 contains address bus
@ assumption: r3 contains SIO_BASE
@ assumption: r5 contains memory pointer (RAM/ROM)
@ using r7
@ --> 20cy

   @ r7 = 0x00FF0000
   movs  r7, #0xFF                           @ 1cy
   lsls  r7, r7, #16                         @ 1cy

   @ data bus direction: write
   str   r7, [r3, #SIO_GPIO_OE_SET_OFFSET]   @ 1cy

   @ remove data bus data from GPIO shadow
   bics  r0, r0, r7   @ r7 = r0 & ~r6        @ 1cy

   @ get data from memory
   ldrb  r2, [r7, r1]                        @ 1cy

   @ combine with GPIO shadow
   lsls  r7, r2, #16                         @ 1cy
   orrs  r0, r0, r7                          @ 1cy
   str   r0, [r3, #SIO_GPIO_OUT_OFFSET]      @ 1cy
                                             @ --> 28cy

   @ this write should occur at least 20ns (3 cycles) before CLK goes
   @ low having 33 cycles for each CLK phase, it means at cycle 30 or
   @ earlier. we are at cycle 28 here.
   @ "mission accomplished. congratulations!"
   @    -- impossible mission

@ --> 28cy
.Lend_of_high_at_28cy:
   @ just pad the remaining 4 cycles
   delay 4                                   @ 4cy

@ -------------------------------------------------------------------------
@ end of clock low phase
@ -------------------------------------------------------------------------

   @ r4 = 65C02 CLK, r3 = SIO_BASE
   str   r4, [r3, #SIO_GPIO_OUT_CLR_OFFSET]  @ 1cy

@ =========================================================================
@ high clock phase starts here
@ =========================================================================

   @ first check if something needs to be send to other core
   cmp   r6, #0                              @ 1cy
   bne   .Lno_multicore_fifo                 @ 1cy --> 3cy

@ -------------------------------------------------------------------------
@ Send GPIO state via FIFO
@ -------------------------------------------------------------------------
@ assumption: r0 contains GPIO data
@ assumption: r3 contains SIO_BASE
@ --> 2cy

   @ldr   r7, =SIO_FIFO_ST_RDY_BITS           @ 2cy
   @ we need to speed up the instruction above
   @ since SIO_FIFO_ST_RDY_BITS = 0x00000002
   @ r4 = 0x02000000
   @ sanity check below
#if SIO_FIFO_ST_RDY_BITS != (BUS_CONFIG_mask_clock >> 24)
#error SIO_FIFO_ST_RDY_BITS/BUS_CONFIG_mask_clock are not as expected
#endif
   lsrs  r7, r4, #24                         @ 1cy
   ldr   r6, [r3, #SIO_FIFO_ST_OFFSET]       @ 1cy
   ands  r7, r6                              @ 1cy
   @ timing error here
   @ in case of a full fifo, 65c02 cycle falls short 1 RP2040 cycle
   @ as this should not happen (but could), this must be accepted
   beq   .Lmulticore_fifo_done               @ 1cy --> 7cy
   str   r0, [r3, #SIO_FIFO_WR_OFFSET]       @ 2cy
                                             @ --> 8cy
.Lmulticore_fifo_done:

@ -------------------------------------------------------------------------
@ Cycle counter and logging
@ -------------------------------------------------------------------------
@ assumption: r0 contains bus data
@ assumption: r4 contains clock mask
@ assumption: r8 contains cycle counter
@ using: r6
@ using: r7
@ --> 8cy

   @ CLK is pin 26, RDY is pin 27
   lsls  r7, r4, #1                          @ 1cy
   ands  r7, r0                              @ 1cy
   beq   .Lnocount                           @ 1cy --> 12cy

   movs  r7, #1                              @ 1cy
   @ high register form
   add   r8, r7                              @ 1cy
   mov   r7, r8                              @ 1cy
   @ store cycle counter for C
   ldr   r5, =cycle_counter                  @ 2cy
   str   r7, [r5]                            @ 2cy
   @ move bits up to discard bits and get them down again
   @ --> and #0x00000FFF
   lsls  r7, r7, #20                         @ 1cy
   lsrs  r7, r7, #20                         @ 1cy
   @ align index to 32bit addressing
   lsls  r7, r7, #2                          @ 1cy
   ldr   r5, =bus_log                        @ 2cy
   str   r0, [r5, r6]                        @ 2cy
   b     mainloop                            @ 2cy --> 27cy

@ --> 12cy
.Lnocount:
   delay 13                                  @ 13cy
   b     mainloop                            @ 2cy --> 27cy

@ --> 3cy
.Lno_multicore_fifo:
   delay 4                                   @ 4cy
   b     .Lmulticore_fifo_done               @ 2cy --> 9cy
