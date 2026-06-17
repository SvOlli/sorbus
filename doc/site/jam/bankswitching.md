
# Bankswitching

The sorbus computer only provides a ROM area of $E000-$FFFF, so
bankswitching is required to provide more than 8kB. OSI BASIC alone
requires >7.5kB. So bankswitching is required to access more than just
the 8kB of ROM banked into memory.

The bankswitching is implemented in the same fashion as on the first
bankswitching cartridges for the Atari 2600 VCS: the whole memory block
is switched (on the Sorbus by writing a value to a specific register,
see below).

## The BIOS Region

The last page of memory ($FF00-$FFFF) has a special function: this is
identical in every bank (or at least similar in specific cases like the
NMOS 6502 monitor). There also is a function (BRK based) provided to
copy the data from ROM to RAM, so kernel functionality can be executed
even from RAM.

## Bankswitching Register

Bankswitching is implemented using a simple register @ $DF00, with the
following values:

- $00: switch area to RAM (required for CP/M, helpful for other things)
- $01: main kernel bank with BRK handler, etc.
- every valid bank >$02: helper function, BASIC, etc.
- invalid banks will switch to bank $01

## Overview

| Method    | From Bank   | To Bank     | Note                                               |
| --------- | ----------- | ----------- | -------------------------------------------------- |
| BRK       | any + RAM   | kernel($01) | RAM only works when BIOS is copied to RAM          |
| bankstart | kernel($01) | any         | starts payload at $E000, e.g. BASIC                |
| bankjsr   | kernel($01) | tools($02)  | very transparent design, expandable to other banks |
| bankjmp   | any         | any         | very quirky in usage, cannot be used for $E000     |
| bootblock | kernel($01) | RAM         | used for CP/M and other bootblocks                 |

So, every method has been taylored to a specific usecase. `bankjsr` and
`bankjmp` require a jumptable at the beginning of the destination bank,
because it's not possible to hand labels between kernel banks.

### Bankswitching Variant 1: BRK Software Interrupts

The kernel uses the software interrupt BRK with an operand to trigger
kernel functions. The code that handles that is non-trivial but allows
for more freedom within the development of the kernel, as things can
be moved around with less implications.

Using the function to wait for a keypress and return it converted to
uppercase runs like this:

```asm65c02
   brk   #$01           ; typically written as "int  CHRINUC" in source

   ; read IRQ vector jumping to
   jmp   (UVIRQ)        ; User Vector for IRQ ($DF78) normally to IRQCHECK

IRQCHECK:
   sta   BRK_SA         ; let's figure out the source of the IRQ
   pla                  ; get processor status from stack
   pha                  ; and put it back
   and   #$10           ; check for BRK bit
   bne   _isbrk
   lda   BRK_SA         ; restore saved accumulator
   jmp   (UVNBI)        ; user vector for non-BRK IRQ ($DF7C)
_isbrk:
   lda   BANK           ; get current bank
   sta   BRK_SB         ; save it
   stx   BRK_SX
   sty   BRK_SY
   tsx
   lda   $0103,x        ; offset of current stack pointer is 1+2
   sta   TMP16+1        ; at offset 0 is processor status (got called via BRK)
   lda   $0102,x        ; get the return address from stack
   sta   TMP16+0        ; and write it to temp vector

   bne   :+
   dec   TMP16+1
:
   dec   TMP16+0
   lda   (TMP16)        ; finally get BRK operand!
                        ; needs to be read without bank change
   sta   ASAVE          ; misuse asave to store BRK operand for user handler
   ldx   #$01           ; will be restored by brkjump
   stx   BANK           ; switch to first ROM bank
   jsr   brkjump        ; to call the subroutine selector

brkjump:
   cmp   #jumptablesize
   bcc   @okay          ; sanity check: if BRK operand out of scope
   lda   #$00           ; reset, user BRK can lda (TMP16) to get BRK operand
@okay:
   asl                  ; BRK >=$80 will always be =$00
   tax
   lda   @jumptable+1,x ; get address from jump table
   pha
   lda   @jumptable+0,x
   pha
   tsx
   lda   $0105,x        ; get processor state from stack
   pha                  ; and save it for rti below
   ldx   BRK_SX         ; get stored X
   lda   BRK_SA         ; get stored A
   rti                  ; jump to handling subroutine

   ; here's the routine to be called
chrinuc:
   ; wait for character from UART and make it uppercase
   jsr   CHRIN
   bcs   chrinuc
   jsr   uppercase
   sta   BRK_SA         ; when called via BRK, this is required
   rts

   ; now return from brkjump
   php
   ldy   BRK_SY         ; get stored Y
   pla
   sta   BRK_SY         ; use stored Y now for storing P
   ldx   BRK_SX         ; get stored X
   lda   BRK_SB         ; get stored BANK
   sta   BANK           ; restore saved bank
   pla                  ; drop old P from stack
   lda   BRK_SY         ; get P from store
   pha                  ; put it on the stack, so flags can be returned
   lda   BRK_SA         ; get stored accumulator
   rti                  ; return to calling code
```
As an interrupt is intended to be handled without changing the registers,
this is here the case as well (except for processor status). If an
interrupt want to "return" a value, this has be written specifically
into the memory addresses where those values are stored: BRK_SA, BRK_SX
and BRK_SY.

This also shows that BRKs cannot be nested. However they are used in
other banks than the kernel to call functions within the kernel bank.

### Bankswitching Variant 2: Core Functionality

This is very blunt. The assumption is that at every bank at $E000 some
basic function is provided:

- $01: kernel reset
- $02: handler for 65816 native interrupts
- $03: OSI BASIC

So, a short routine in the BIOS is implemented for jumping to a specific
bank. This includes extra code to switch 65816 to 8-bit mode, so the
kernel routines for native interrupts and reset are guaranteed to work.
(Even when a softreset via jmp ($fffc) is performed in native mode.)

```asm65816
_unhandled:
   sep   #$30           ; set MX to 8-bit mode, like 65C02
                        ; 65(S)C02 will execute: NOP #$30
                        ; 65CE02 will execute LDA ($30,S),Y
   ldx   #UNH65816_BANK
   bra   bankstart
_reset:                 ; $FFFC points here
   sep   #$30           ; set MX to 8-bit mode, like 65C02
                        ; 65(S)C02 will execute: NOP #$30
                        ; 65CE02 will execute LDA ($30,S),Y
   ldx   #KERNEL_BANK   ; reset routine is per definition in kernel
bankstart:
   stx   BANK           ; select bank from X
   jmp   $E000          ; jump to table index, table will be used top down
```
The accumulator is left untouched, just in case it is required to "pass
something along". The "bankgotoE000" function can also be used to switch
from any bank to any bank, with the exception that to bank $00 is not
guarateed to work as that is RAM.

Running a bootblock uses a different approach: writing a small stub at
the beginning of the stack ($0100) and jumping there. This also includes
a small routine at @ $0100 directly which copies the BIOS from ROM to
RAM.

Also a valid bootblock typically starts like this:
```asm65c02
   jmp   realstart      ; jump to read start
   .byte "SBC23"        ; magic value for a valid boot block
```
The jmp at the beginning is just a suggestion, any three byte code can
go in there.

### Bankswitching Variant 3: Kernel Jumps To Subroutine In Other Bank

Here the intention is different: code from bank $01 jumps into a
subroutine into bank $02 and returns from there. It should be possible
to expand this to other banks as well, but this is not implemented
right now.

```asm65c02
firstbankjsr:
subroutine1:   ; bank 2: $E003
   nop
subroutine2:   ; bank 2: $E006
   nop
subroutine3:   ; bank 2: $E009
   sta   ASAVE
   php
   pla
   ; stack now contains: RETL RETH
   sta   PSAVE
   phx
   ; stack now contains: X RETL RETH
   ; we want to get these: ^^^^ ^^^^
   ; RET points at "yy" the the byte sequence 20 xx yy
   ; however, we want "xx", so decrement by 1
   tsx
   lda   $0103,x
   sta   TMP16+1
   lda   $0102,x
   bne   :+
   dec   TMP16+1
:
   dec
   sta   TMP16+0

   ; now we know where the call originated from
   lda   (TMP16)
   sec
   sbc   #(<firstbankjsr)-1 ; adjust for start of nopslide and JSR "offset"
   ; here some further evaluation could take place, if a jsr to a bank other
   ; than TOOLS_BANK is required
   sta   TMP16+0
   asl                  ; A needs to be < $FF/3 = $55, so no clc needed
   adc   TMP16+0
   dec
   sta   TMP16+0        ; TMP16+0 now holds $ff,$02,...
   plx
   ; stack now contains: RETL RETH

   lda   #>(banksubret-1)
   pha
   lda   #<(banksubret-1)
   pha
   ; stack now contains:  banksubretL banksubretH RETL RETH

   lda   #>ROMSTART
   pha
   lda   TMP16+0
   pha

   ; stack now contains: banksubL banksubH banksubretL banksubretH RETL RETH
   lda   PSAVE
   pha
   ; stack now contains: P banksubL banksubH banksubretL banksubretH RETL RETH

   phx
   ; stack now contains: X P banksubL banksubH banksubretL banksubretH RETL RETH

   ldx   #TOOLS_BANK
   lda   ASAVE
   jmp   banksubgo
   ; PC now at banksubgo

banksubgo:
   stx   BANK
   plx
   ; stack now contains: P banksubL banksubH banksubretL banksubretH RETL RETH
   plp
   ; stack now contains: banksubL banksubH banksubretL banksubretH RETL RETH
   rts
   ; PC now at banksub
   ; stack now contains: banksubretL banksubretH RETL RETH

banksub:
   ; [... running subroutine ...]
   rts
   ; PC now at banksubret
   ; stack now contains: RETL RETH

banksubret:
   php
   ; stack now contains: P RETL RETH
   phx
   ; stack now contains: X P RETL RETH
   ldx   #KERNEL_BANK
   ; called by kernel switch to bank for subroutine call
   ; php and pha are done there
banksubgo:
   stx   BANK
   plx
   ; stack now contains: P RETL RETH
   plp
   ; stack now contains: RETL RETH
   rts
```
Similar to the BRK routine everything is preserved during bankswitching:
all registers, as well as processor status. Just like it is expected
from a "normal" JSR. But as there a few memory addresses are modified as
part of the setup process. However, those can be used within the
subroutines.

### Bankswitching Variant 4: Kernel Jumps To Other Bank Without Return

This can be implemented by "hijacking" parts of the Variant 3. Basic idea
is to reuse "banksubgo", a nice implementation is still work be done.

```asm65c02
bankjmp:
   ; A=entrypoint: $E0xx : xx = A
   ; X=bank
   ; ASAVE=A on entry
   ; PSAVE=X on entry
   php
   ; stack now contains: P
   sta   TMP16+0        ; saving A=entrypoint-1
   pla
   ; stack now contains: (nothing)
   sta   TMP16+1        ; saving P

   lda   #>ROMSTART
   pha
   ; stack now contains: JMPH
   lda   TMP16+0
   dec                  ; adjusting for RTS
   pha
   ; stack now contains: JMPL JMPH
   lda   TMP16+1        ; getting P
   pha
   ; stack now contains: P JMPL JMPH
   lda   PSAVE          ; getting X for function call
   pha
   ; stack now contains: X P JMPL JMPH
   lda   ASAVE          ; getting A for function call
   jmp   banksubgo

banksubgo:
   stx   BANK
   plx
   ; stack now contains: P RETL RETH
   plp
   ; stack now contains: RETL RETH
   rts
```
The "banksubgo" part is the same code used in "banksubret" above.
