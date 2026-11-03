
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
@ Set up
@ =========================================================================
.syntax unified
.cpu cortex-m0plus
.thumb

@ =========================================================================
@ External C variables
@ =========================================================================
.extern irq_write_handlers
.extern irq_read_handlers

@ =========================================================================
@ Assembly code section
@ =========================================================================
.section .text.fifo_irq_handler, "ax"
.align 2

.global fifo_irq_handler_asm
.type fifo_irq_handler_asm, %function

fifo_irq_handler_asm:
   @ assuming input in SIO_FIFO is sanititized!!!
   @ only in range of 0xDF00-0xDF7F for write / 0xDF00-0xDF0F for read
   @ since we are calling a C function, we need to save lr
   # also r4, for alignment and we beed something for the SIO_BASE
   push  {r4, lr}                            @ 3cy
.Lagain:
   ldr   r4, =SIO_BASE                       @ 2cy
   ldr   r0, [r4, #SIO_FIFO_ST_OFFSET]       @ 1cy
   lsls  r0, r0, #31                         @ 1cy
   @ is there data in the queue? -> process it
   bpl   .Lprocess                           @ 1/2cy
   @ write any value to clear the error flags / acknowledge IRQ
   str   r0, [r4, #SIO_FIFO_ST_OFFSET]       @ 1cy
   @ leave the handler
   pop   {r4, pc}                            @ 4cy
.Lprocess:
   @ fetch FIFO entry
   @ r0 is also first parameter for C function
   ldr   r0, [r4, #SIO_FIFO_RD_OFFSET]       @ 1cy
   @ set N(bit31) to R=1/W=0(bit24), only flag matters
   lsls  r1, r0, #7                          @ 1cy
   bmi   .Lbusread                           @ 1/2cy
   @ write: 4 bytes get a common handler => 32 entries in table
   @ as by definition, addressbus may only be 0xDF00-0xDF0F here
   movs  r2, #0x7c                           @ 1cy
   ands  r2, r0                              @ 1cy
   ldr   r1, =irq_write_handlers             @ 2cy
   b     .Lcallexec                          @ 2cy
.Lbusread:
   @ read: a handler for each single byte of 0xDF00-0xDF0F
   @ write: 4 bytes get a common handler
   @ as by definition, addressbus may only be 0xDF00-0xDF0F here
   uxtb  r2, r0                              @ 1cy
   lsls  r2, r2, #2                          @ 1cy
   ldr   r1, =irq_read_handlers              @ 2cy
.Lcallexec:
   @ at this point registers should be
   @ r1 = base of jumptable
   @ r2 = offset in bytes
   ldr   r3, [r1, r2]                        @ 2cy
   cmp   r3, #0x00                           @ 1cy
   beq   .Lagain                             @ 1/2cy
   @ r0 = uint32_t gpios
   @ r1 = uint16_t address bus
   @ r2 = uint8_t  data bus
   uxth  r1, r0                              @ 1cy
   lsrs  r2, r0, #16                         @ 1cy
   uxtb  r2, r2                              @ 1cy
   blx   r3
   b     .Lagain                             @ 2cy

