
# MCP Examples

Here are some examples how you can see what the CPU does during every
single clock cycle. Please note, that writing to that stack will happen
at different addresses within the $0100-$01ff area, as the stackpointer
is not initialized.

## Commands used:
- `s <cycles>`
    - let the CPU run for &lt;cycles&gt; cycles / steps
    - the parameter is decimal
- `f <start> <end> <value>`
    - fill the memory from &lt;start&gt; to &lt;end&gt; with the value
      &lt;value&gt;
    - all parameter are hexadecimal
- `reset <cycles>`
    - pull the reset line to low for &lt;cycles&gt; cycles
    - the parameter is decimal
- `: <start> <value> (<value> ...)`
    - write value(s) to memory beginning at start
    - &lt;start&gt; and the colon (`:`) may be swapped
    - all parameter are hexadecimal

Commands are case sensitive, hexadecimal numbers are not.

### Output:

Let's use a fictional example to explain the output:
```txt
100:fffc r 00 R  >
```

From left to right:

- `100:`
    - number of cycles left to run: if number > 999 then 999 will be displayed.
- `fffc`
    - current state of address-bus → address being accessed.
- `r`
    - data is being read. (Only other value here would be "w" for write.)
- `00`
    - current state of data-bus → value being read or written.
- `R`
    - other CPU lines being triggered by pulling low: R=reset (triggered in
    this case), others are (not triggered here): N=nmi, I=irq.
- `>`
    - the monitor command prompt, you can type while the CPU is running.


## Lesson 1: reset and NOP

This lesson will run on 6502, 65C02 and 65816.

### Commands to enter:

```txt
f 0000 ffff ea
reset 5
s 17
```

### Output:
```txt
 16:eaeb r ea R  >
 15:eaec r ea R  >
 14:eaec r ea R  >
 13:eaec r ea R  >
 12:eaec r ea R  >
 11:eaec r ea    >
 10:eaec r ea    >
  9:eaec r ea    >
  8:01f7 r ea    >
  7:01f6 r ea    >
  6:01f5 r ea    >
  5:fffc r ea    >
  4:fffd r ea    >
  3:eaea r ea    >
  2:eaeb r ea    >
  1:eaeb r ea    >
  0:eaec r ea    >
```

<div class='explain_wrap' markdown>
### Explanation:

(hover to show)
<div class='explain_main' markdown>
- During cycles 16-12 the CPU does nothing, since reset is triggered.
- During cycles 11-9 the CPU starts running internally.
- During cycles 8-6 stack access is happening, because reset is implemented
  similar to an interrupt. The behavour until here might differ from chip to chip!
- During cycles 5-4 the reset vector is read, low byte first.
- During cycles 3-2 a NOP OpCode are processed. Since the OpCode takes two
  cycles to execute, the CPU triggers a dummy read on the following address.
  Just in case, there is a parameter for an instruction to fetch.
- During cycles 1-0 the next NOP OpCode is processed.
</div></div>


## Lesson 2: another kind of NOP

This only run on 65C02 and 65SC02. The OpCode used has no equivalent on
6502, 65CE02 and 65816.

### Commands to enter:

```txt
f 0000 ffff 33
reset 5
s 17
```

### Output:
```txt
[...skipping unimportant lines...]
  5:fffc r 33    >
  4:fffd r 33    >
  3:3333 r 33    >
  2:3334 r 33    >
  1:3335 r 33    >
  0:3336 r 33    >
```

<div class='explain_wrap' markdown>
### Explanation:

(hover to show)
<div class='explain_main' markdown>
- During cycles 5-4 the reset vector is read, low byte first.
- During cycles 3-0 undocumented OpCodes are processed. This is handles
  differently on each variant:
    - (6502: the CPU executes two different instructions at once: ROL and AND.)
    - (65816: since each OpCode is defined, there is no example on how undefined
      OpCodes work.)
    - 65C02 and 65SC02: each undocumented OpCode is executing a "NOP", but the
      parameter can differ from OpCode to OpCode. This is a very interresting
      one: no dummy reads are executed, this instruction only requires a single
      clock cycle. This is not possible on the 6502 or 65816.
    - (65CE02 will execute a long range BMI; however, the NOP only takes a
      single cycle there)
</div></div>


## Lesson 3: a simple program

If you ran lessons 1 and/or 2, power cycle the Sorbus Computer. This ensures
that the program executed will be in memory. This lesson will run on 6502,
65C02 and 65816. Also take a good look at the output of the memory dump. Does
that look familiar to you?

### Commands to enter:

```txt
f 1000 1100 00
: fffc 00 04
reset 5
freq 1000
s 345
freq 5
m 1000
```

### Output:
```txt
[...skipping a lot of output...]
  5:0421 r b0    >
  4:0422 r fe    >
  3:0423 r e8    >
  2:0421 r b0    >
  1:0422 r fe    >
  0:0423 r e8    >m 1000
: 1000  00 01 01 02 03 05 08 0d  15 22 37 59 90 e9 00 00
  0:0423 r e8    >
```

<div class='explain_wrap' markdown>
### Explanation:

(hover to show)
<div class='explain_main' markdown>
The rest of the output show that the program running is in an endless loop
once calculation is done. The output is of course the start of the
[Fibonacci sequence](https://en.wikipedia.org/wiki/Fibonacci_sequence)
as far as it can be calculated in 8 bits. The code is stopping when the
add overflows.

Source code: (excluding first JMP instruction)
```asm6502
   clc
   cld
   ldx   #$01
   stx   BUFFER+1
   dex
   stx   BUFFER

@loop:
   lda   BUFFER,x
   inx
   adc   BUFFER,x
   bcs   *
   inx
   sta   BUFFER,x
   dex
   bne   @loop
```
</div></div>


## Lesson 4: subroutine

This runs on all CPUs: 65C02, 6502 and 65816. No need to powercycle.

### Commands to enter:

```txt
: fffc 03 04
reset 5
s 35
```

### Output:
```txt
 23:fffc r 03    >
 22:fffd r 04    >
 21:0403 r 4c    >
 20:0404 r 2a    >
 19:0405 r 04    >
 18:042a r 20    >
 17:042b r 57    >
 16:01dc r ea    >
 15:01dc w 04    >
 14:01db w 2c    >
 13:042c r 04    >
 12:0457 r ea    >
 11:0458 r ea    >
 10:0458 r ea    >
  9:0459 r 60    >
  8:0459 r 60    >
  7:045a r 00    >
  6:01da r ea    >
  5:01db r 2c    >
  4:01dc r 04    >
  3:042c r 04    >
  2:042d r 4c    >
  1:042e r 2d    >
  0:042f r 04    >
```

<div class='explain_wrap' markdown>
### Explanation:

(hover to show)
<div class='explain_main' markdown>
- After the reset, the first instruction is a JuMP, so I can easily modify
  the code without the need to change the start address.
- During the cycles 18-13 the CPU executes a Jump to SubRoutine:
    - 18: fetch the JSR OpCode
    - 17: fetch low byte of jump target address
    - 16: dummy read of stack
    - 15: write high byte of return address minus 1 to stack
    - 14: write low byte of return address minus 1 to stack
    - 13: fetch high byte of jump target address and perform jump
- The ReTurn from Subroutine starts at cycle 8 (cycle 9 is the dummy read
  from the NOP OpCode at 10):
    - 8: fetch the RTS OpCode
    - 7: dummy read non-existant parameter
    - 6: dummy read from stack
    - 5: fetch low byte of return address minus 1 from stack
    - 4: fetch high byte of return address minus 1 from stack
    - 3: dummy read from memory to adjust program counter to correct return
      address and jump back
    - 2-0: endless loop
- Interesting how the return address minus one gets written to stack and
  how it is compensated by the return call. This seems to be caused by the
  split of reading the low and high bytes of the target address.

Source code: (excluding first JMP instruction)
```asm6502
   jsr   jsrtest
   jmp   *

jsrtest:
   nop
   nop
   rts
   brk
```
</div></div>


## Lesson 5: data on stack

This runs on all CPUs: 65C02, 6502 and 65816. No need to powercycle.

### Commands to enter:

```txt
: FFFC 06 04
reset 5
s 30
```

### Output:
```txt
 18:fffc r 06    >
 17:fffd r 04    >
 16:0406 r 4c    >
 15:0407 r 30    >
 14:0408 r 04    >
 13:0430 r a9    >
 12:0431 r 42    >
 11:0432 r 48    >
 10:0433 r 68    >
  9:01f1 w 42    >
  8:0433 r 68    >
  7:0434 r 4c    >
  6:01f0 r ea    >
  5:01f1 r 42    >
  4:0434 r 4c    >
  3:0435 r 34    >
  2:0436 r 04    >
  1:0434 r 4c    >
  0:0435 r 34    >
```

<div class='explain_wrap' markdown>
### Explanation:

(hover to show)
<div class='explain_main' markdown>
- After the reset, the first instruction is a JuMP, so I can easily modify
  the code without the need to change the start address.
- Next the accumulator is loaded with a recognizable value ($42).
- Push the value on the stack
    - 11: fetch the PHA OpCode
    - 10: dummy argument read
    - 9: push value to the stack
- Pull the value from the stack
    - 8: fetch the PLA OpCode
    - 7: dummy argument read
    - 6: dummy read from stack to adjust stack pointer
    - 5: pull value from the stack
- End program
    - 4-0: endless loop

Source code: (excluding first JMP instruction)
```asm6502
   lda   #$42
   pha
   pla
   jmp   *
```
</div></div>


## Lesson 6: interrupt via IRQ line

This runs on all CPUs: 65C02, 6502 and 65816. No need to powercycle.

### Commands to enter:

```txt
: FFFC 09 04
reset 5
s 30
irq 8
s 36
```

### Output:
```txt
 13:0437 r a9    >
 12:0438 r 5d    >
 11:0439 r 8d    >
 10:043a r fe    >
  9:043b r ff    >
  8:fffe w 5d    >
  7:043c r a9    >
  6:043d r 04    >
  5:043e r 8d    >
  4:043f r ff    >
  3:0440 r ff    >
  2:ffff w 04    >
  1:0441 r ea    >
  0:0442 r ea    >irq 8
 35:0442 r ea   I>
 34:0443 r ea   I>
 33:0443 r ea   I>
 32:0444 r 58   I>
 31:0444 r 58   I>
 30:0445 r 4c   I>
 29:0445 r 4c   I>
 28:0446 r 45   I>
 27:0447 r 04    >
 26:0445 r 4c    >
 25:0445 r 4c    >
 24:01eb w 04    >
 23:01ea w 45    >
 22:01e9 w 21    >
 21:fffe r 5d    >
 20:ffff r 04    >
 19:045d r 48    >
 18:045e r ea    >
 17:01e8 w 04    >
 16:045e r ea    >
 15:045f r ea    >
 14:045f r ea    >
 13:0460 r 68    >
 12:0460 r 68    >
 11:0461 r 40    >
 10:01e7 r ea    >
  9:01e8 r 04    >
  8:0461 r 40    >
  7:0462 r 00    >
  6:01e8 r 04    >
  5:01e9 r 21    >
  4:01ea r 45    >
  3:01eb r 04    >
  2:0445 r 4c    >
  1:0446 r 45    >
  0:0447 r 04    >
```

<div class='explain_wrap' markdown>
### Explanation:

(hover to show)
<div class='explain_main' markdown>
- after the reset, the first instruction is a JuMP, so I can easily modify
  the code without the need to change the start address.
- after that the IRQ vector gets set up, until "IRQ 9" is entered.
- main program waits for interrupts
    - 35-30: processing OpCodes NOP, NOP, and CLI (CLear Interrupt disable)
    - 29-27: processing endless loop JuMP, and wait for interrupt to trigger
- *interrupt is triggered*
    - 26-25: two dummy reads: interrupt is triggered while fetching OpCode
    - 24-23: write high byte and low byte of return address to the stack
    - 22: write processor status to stack
    - 21-20: read interrupt vector
    - 19-9: write to and read from stack (like previous example)
- returning from interrupts
    - 8: fetch RTI (ReTurn from Interrupt) OpCode
    - 7: dummy argument read
    - 6: dummy to adjust stack pointer
    - 5: read processor status from stack
    - 4-3: read low byte and high byte of return address from stack
- returned to main program
    - 2-0: process endless loop interrupt at 28-26
- it is noteworthy, that return address is not "minus one" as in the JSR

Source code: (excluding first JMP instruction)
```asm6502
   lda   #<irqtest
   sta   $fffe
   lda   #>irqtest
   sta   $ffff
   nop
   nop
   nop
   cli
   jmp   *

irqtest:
   pha
   nop
   nop
   pla
   rti
   brk
```
</div></div>


## Lesson 7: interrput via BRK OpCode

This runs on all CPUs: 65C02, 6502 and 65816. No need to powercycle.

### Commands to enter:

```txt
: FFFC 0C 04
reset 5
s 54
```

### Output:
```txt
 37:0448 r a9    >
 36:0449 r 5d    >
 35:044a r 8d    >
 34:044b r fe    >
 33:044c r ff    >
 32:fffe w 5d    >
 31:044d r a9    >
 30:044e r 04    >
 29:044f r 8d    >
 28:0450 r ff    >
 27:0451 r ff    >
 26:ffff w 04    >
 25:0452 r 00    >
 24:0453 r ea    >
 23:01ee w 04    >
 22:01ed w 54    >
 21:01ec w 35    >
 20:fffe r 5d    >
 19:ffff r 04    >
 18:045d r 48    >
 17:045e r ea    >
 16:01eb w 04    >
 15:045e r ea    >
 14:045f r ea    >
 13:045f r ea    >
 12:0460 r 68    >
 11:0460 r 68    >
 10:0461 r 40    >
  9:01ea r ea    >
  8:01eb r 04    >
  7:0461 r 40    >
  6:0462 r 00    >
  5:01eb r 04    >
  4:01ec r 35    >
  3:01ed r 54    >
  2:01ee r 04    >
  1:0436 r ea    >
  0:0437 r ea    >
```

<div class='explain_wrap' markdown>
### Explanation:

(hover to show)
<div class='explain_main' markdown>
- after the reset, the first instruction is a JuMP, so I can easily modify
  the code without the need to change the start address
- After that the IRQ vector gets set up, and the BRK OpCode is processed
    - 37-26: set up IRQ vector which is also used for the software interrupt
      using BRK (BReaK)
    - 25: process BRK Opcode
    - 24: dummy argument read
- process BRK interrupt
    - 23-22: write high byte and low byte of return address to the stack
      &nbsp; <u class="underlined">NOTE:</u> the return address is the one after the dummy
      read, the documentation states that the BRK OpCode uses two bytes
    - 21: write processor status to stack
    - 20-19: read interrupt vector
    - 18-8: write to and read from stack (like previous example)
- return from interrupt
    - 7: fetch RTI (ReTurn from Interrupt) OpCode
    - 6: dummy argument read
    - 5: dummy to adjust stack pointer
    - 4: read processor status from stack
    - 3-2: read low byte and high byte of return address from stack
- returned from interrupt
    - 1-0: process the following NOP OpCode

Source code: (excluding first JMP instruction)
```asm6502
   lda   #<irqtest
   sta   $fffe
   lda   #>irqtest
   sta   $ffff
   brk
   nop
   nop
   nop
   jmp   *

irqtest:
   pha
   nop
   nop
   pla
   rti
   brk
```
</div></div>
