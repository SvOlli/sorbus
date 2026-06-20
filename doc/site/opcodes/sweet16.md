
# SWEET16

The SWEET16 is a virtual CPU interpreter, written by Stephen Wozniak as
part of the first Apple ][ Computer. It was removed on later machines
but still is an interesting piece of software, originally implemented in
just 393 bytes.

It was described in [Byte Magazine Volume 02 Number 11 (1977)
](https://archive.org/download/BYTE_Vol_02-11_1977-11_Sweet_16/BYTE_Vol_02-11_1977-11_Sweet_16.pdf).
Most of the opcode description was transscribed from there.

The source code published there has been adapted to better fit the Sorbus
Computer. 

## Implementation differences

There are a few things that are done differently in the Sorbus port of
SWEET16 as compared to the original.

- SWEET16 is invoked using BRK #$10 instead of JSR $xxxx
- BK does not execute a 6502 BRK, but triggers the TRAP to meta-mode
  (for details see [BK description below](#bk))
- when R12 is zero (stack for [BS instruction](#bs)), it will be
  initialized to $0100 upon startup of SWEET16

## Special Registers

The SWEET16 has a focus on registers compared to the 6502 which is more
focused on memory stack. Five of the sixteen 16-bit registers provided
by SWEET16 have a special function bound to them.

| Name | Function |
| ---- | -------- |
| R0   | accumulator |
| R12  | subroutine stack pointer |
| R13  | stores the result of all comparison operations for branch testing |
| R14  | status register |
| R15  | program counter |

## Register Ops

### SET Rn

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| SET Rn, Constant | $1n | $lb $hb | Set |

The 2 byte constant is loaded into Rn (n = 0 to F, hexadecimal) and
branch conditions set accordingly. The carry is cleared.

Example:
```
15 34 A0       SET   R5,$A034 ; R5 now contains $A034
```

### LD Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| LD Rn | $2n | Load |

The ACC (R0) is loaded from Rn and branch conditions set according to
the data transferred. The carry is cleared and the contents of Rn are
not disturbed.

Example:
```
15 34 A0       SET   R5,$A034
36             LD    R5       ; ACC now contains $A034
```

### ST Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| ST Rn | $3n | Store |

The ACC (R0) is stored into Rn and branch conditions set according to
the data transferred. The carry is cleared and the ACC contents are
not disturbed.

Example:
```
25             LD    R5       ; copy contents
36             ST    R6       ; of R5 to R6
```

### LD @Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| LD @Rn | $4n | Load indirect |

The low order ACC byte is loaded from the memory location whose address
resides in Rn, and the high order byte is cleared. Branch conditions
reflect the final ACC contents which will always be positive and never
minus 1.
The carry is cleared.
After the transfer, Rn is incremented by 1.

Example:
```
15 34 A0       SET   R5,$A034
45             LD   @R5       ; ACC is loaded from memory location $A034
                              ; R5 is incremented to $A035
```

### ST @Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| ST @Rn | $5n | Store indirect |

The low order ACC byte is stored into the memory location whose address
resides in Rn, and the high order byte is cleared. Branch conditions
reflect the 2 byte ACC contents, which are not disturbed.
The carry is cleared.
After the transfer, Rn is incremented by 1.

Example:
```
15 34 A0       SET   R5,$A034 ; load pointers R5 and R6
16 22 90       SET   R6,$9022 ; with $A034 and $9022
45             LDD  @R5       ; move byte from location $A034
56             STD  @R6       ; to location $9022
                              ; both pointers are incremented
```

### LDD @Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| LDD @Rn | $6n | Load double byte indirect |

he low order ACC byte is loaded from the memory location whose address
resides in Rn, and Rn is then incremented by 1. The high order ACC byte
is loaded from the memory location whose address resides in the
(incremented) Rn and Rn is again incremented by 1.
The carry is cleared.

Example:
```
15 34 A0       SET   R5,$A034
65             LDD  @R5       ; the low order ACC byte is loaded from location $A034
                              ; the high order ACC byte from location $A035
                              ; R5 is incremented to $A036
```

### STD @Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| STD @Rn | $7n | Store double byte indirect |

The low order ACC byte is stored into the memory location whose address
resides in Rn, and Rn is then incremented by 1. The high order ACC byte
is stored into the memory location whose address resides in (the
incremented) Rn and Rn is again incremented by 1. Branch conditions
reflect the ACC contents which are not disturbed.
The carry is cleared.

Example:
```
15 34 A0       SET   R5,$A034 ; load pointers R5 and R6
16 22 90       SET   R6,$9022 ; with $A034 and $9022
65             LDD  @R5       ; move double byte from locations $A034 and $A035
76             STD  @R6       ; to locations $9022 and $9023
                              ; both pointers are incremented by 2
```

### POP @Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| POP @Rn | $8n | Pop indirect |

The low order ACC byte is loaded from the memory location whose address
resides in Rn after Rn is decremented by 1 and the high order ACC byte
is cleared. Branch conditions reflect the final 2 byte ACC contents
which will always be positive and never minus 1.
The carry is cleared.
Because Rn is decremented prior to loading the ACC, single byte stacks
may be implemented with the ST @Rn and POP @Rn operations
(Rn is the stack pointer).

Example:
```
15 34 A0       SET   R5,$A034 ; init stack pointer
10 04 00       SET   R0,$0004 ; load 4 into ACC
35             ST   @R5       ; push 4 onto stack
10 04 00       SET   R0,$0005 ; load 5 into ACC
35             ST   @R5       ; push 5 onto stack
10 04 00       SET   R0,$0006 ; load 6 into ACC
35             ST   @R5       ; push 6 onto stack
85             POP  @R5       ; pop 6 off stack into ACC
85             POP  @R5       ; pop 5 off stack into ACC
85             POP  @R5       ; pop 4 off stack into ACC
```

### STP @Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| STP @Rn | $8n | Store pop indirect |

The low order ACC byte is stored into the memory location whose address
resides in Rn after Rn is decremented by 1. Then the high order ACC byte
is stored into the memory location whose address resides in Rn after Rn
is again decremented by 1.
Branch conditions will reflect the 2 byte ACC contents which are not
modified.
STP @Rn and POP @Rn are used together to move data blocks beginning at
the greatest address and working down. Additionally, single byte stacks
may be implemented with the STP @Rn and LOA @Rn ops.

Example:
```
14 24 A0       SET   R4,$A034 ; init pointers
15 22 90       SET   R5,$9022
84             POP  @R4       ; move byte from $A033
95             STP  @R5       ;   to $9021
84             POP  @R4       ; move byte from $A032
95             STP  @R5       ;   to $9020
```

### ADD Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| ADD Rn | $An | Add |

The contents of Rn are added to the contents of the ACC (RO) and the low
order 16 bits of th e sum restored in ACC.
The 17th sum bit becomes the carry and other branch conditions reflect
the final ACC contents.

Example:
```
10 34 76       SET   R0,$7643 ; init R0 (ACC)
11 27 42       SET   R1,$4227 ; and R1
A1             SUB   R1       ; add R1 (sum = $B85B, carry clear)
A0             SUB   R0       ; double the ACC (R0) to $70B6 with carry set
```

### SUB Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| SUB Rn | $Bn | Subtract |

The contents of Rn are subtracted from the ACC contents by performing a
two's complement addition:

ACC = ACC + (Rn ^ $FFFF) + 1

The low order 16 bits of the subtraction are restored in the ACC.
The 17th sum bit becomes the carry and other branch conditions reflect
the final ACC contents. If the 16 bit unsigned ACC contents are greater
than or equal to the 16 bit unsigned Rn contents then the carry is set,
otherwise it is cleared. Rn is not disturbed.

Example:
```
10 34 76       SET   R0,$7643 ; init R0 (ACC)
11 27 42       SET   R1,$4227 ; and R1
A1             SUB   R1       ; subtract R1 (difference = $340D with carry set)
A0             SUB   R0       ; clears the ACC (R0)
```

### POPD @Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| POPD @Rn | $Cn | Pop double indirect byte |

Rn is dec remented by 1 and the high order ACC byte is loaded from the
memory location whose address now resides in Rn. Then Rn is again
decremented by 1 and the low order ACC byte is loaded from the
corresponding memory location. Branch conditions reflect the final ACC
contents.
The carry is cleared.
Because Rn is decremented prior to loading each of the ACC halves,
double byte stacks may be implemented with the STD @Rn and POPD @Rn
operations. (Rn is the stack pointer).

Example:
```
15 34 A0       SET   R5,$A034 ; init stack pointer
10 12 AA       SET   R0,$AA12 ; load $AA12 into ACC
75             STD  @R5       ; push $AA12 onto stack
10 34 BB       SET   R0,$BB34 ; load BB34 into ACC
75             STD  @R5       ; push BB34 onto stack
10 56 CC       SET   R0,$CC56 ; load CC56 into ACC
75             STD  @R5       ; push CC56 onto stack
C5             POPD @R5       ; pop CC56 off stack
C5             POPD @R5       ; pop BB34 off stack
C5             POPD @R5       ; pop AA12 off stack
```

### CPR Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| CPR Rn | $Dn | Compare |

The ACC (R0) conten ts are compared to Rn by performing the 16 bit
binary subtraction ACC - Rn and storing the low order 16 difference bits
in R13 for subsequent branch tests. If the 16 bit unsigned ACC contents
are greater than or equal to the 16 bit unsigned Rn contents then the
carry is set, otherwise it is cleared. No other registers, including ACC
and Rn, are disturbed.

Example:
```
15 34 AO       SET   R5,$A034 ; pointer to memory
16 BF AO       SET   R6,$A0BF ; limit address
10 00 00 LOOP  SET   RO,$0000 ; limit address
75             STD  @R5       ; zero data
25             LD    R5       ; clear 2 locations, increment R5 by 2
D6             CPR   R6       ; compare pointer R5 to limit R6
02 F8          BNC   LOOP     ; loop if carry clear
```

### INR Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| INR Rn | $En | Increment |

The contents of Rn are incremented by 1. The carry is cleared and other
branch conditions reflect the incremented value.

Example:
```
15 34 AO       SET   R5,$A034 ; init R5 (pointer)
10 00 00       SET   RO,$0000 ; zero to RO
55             ST   @R5       ; clears loc A034 and increments R5 to $A035
E5             INR   R5       ; increment R5 to $A036
55             ST   @R5       ; clears location $A036 (not $A035)
```

### DCR Rn

| Name | Opcode | Function |
| ---- | ------ | -------- |
| DCR Rn | $Fn | Decrement |

The contents of Rn are decremented by 1. The carry is cleared and other
branch conditions reflect the decremented value.

Example: (clear nine bytes beginning at loc $A034)
```
15 34 AO       SET   R5,$A034    ; init pointer
14 09 00       SET   R4,$0009    ; init count.
10 00 00       SET   RO,$0000    ; zero ACC.
55       LOOP  ST   @R5          ; clear a mem byte
F4             DCR   R4          ; decrement count
07 FC          BNZ   LOOP        ; loop until zero
```

## Nonregister Ops

### RTN

| Name | Opcode | Function |
| ---- | ------ | -------- |
| RTN | $00 | return to 6502 mode |

Control is returned to the 6502 and program execution continues
immediately following the RTN instruction. The 6502 registers and status
conditions are restored to their original contencts (prior entering
SWEET16 mode).

### BR

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| BR ea | $01 | $rl | branch always |

An effective address (ea) is calculated by adding the signed displacement
byte (rl) to the program counter. The program counter contains the
address of the instruction immediately _following_ the BR, or the address
of the BR operation plus 2. The displacement byte is a signed two's
complement value from -128 to +127. Branch contitions are not changed.
Note that the effective address calculation is identical to that for 6502
relative branches.

Example
```
0300: 01 50          BR    $0352
```

### BNC

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| BNC ea | $02 | $rl | branch if no carry |

A branch to the effective address is taken only if the carry is clear,
otherwise execution resumes as normal with the next instruction.
Branch conditions are not changed.

### BC

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| BC ea | $03 | $rl | branch if carry set |

A branch is effected only if the carry is set.
Branch conditions are not changed.

### BP

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| BP ea | $04 | $rl | branch if plus |

A branch is effected only if the prior "result" (or most recently
transferred data) was positive.
Branch conditions are not changed.

Example:
```
15 34 A0       SET   R5,$A034 ; init pointer
14 3F A0       SET   R4,$A03F ; init limit
10 00 00 LOOP  SET   R0,$0000
55             ST   @R5       ; clear memory byte increment R5
24             LD    R4       ; compare limit to
D5             CPR   R5       ; pointer
04 F8          BP    LOOP     ; loop until done
```

### BM

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| BM ea | $05 | $rl | branch if minus |

A branch is effected only if the prior "result" was minus (negative,
MSB=1).
Branch conditions are not changed.

### BZ

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| BZ ea | $06 | $rl | branch if zero |

A branch is effected only if the prior "result" was zero.
Branch conditions are not changed.

### BNZ

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| BNZ ea | $07 | $rl | branch if nonzero |

A branch is effected only if the prior "result" was nonzero.
Branch conditions are not changed.

### BM1

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| BM1 ea | $08 | $rl | branch if minus 1 |

A branch is effected only if the prior "result" was minus 1 ($FFFF
hexadecimal).
Branch conditions are not changed.

### BNM1

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| BNM1 ea | $09 | $rl | branch if not minus 1 |

A branch is effected only if the prior "result" was not minus 1 ($FFFF
hexadecimal).
Branch conditions are not changed.

### BK

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| BK ea | $0A | $xx | break |

This differs from the original implementation: instead of issuing a
6502 BRK (break) instruction, the CPU is halted and the machine switches
to [meta-mode](../jam/meta_mode.md). This is done by writing the argument
value ($xx) to the trap register $DF01. This way the BK causing the stop
can be easier identified.

Note: since the Sorbus cannot run a BRK with a BRK and SWEET16 is invoked
using a BRK, the original behavior cannot be implemented.

### RS

| Name | Opcode | Function |
| ---- | ------ | -------- |
| RS | $0B | return from SWEET16 subroutine |

RS terminates execution of a SWEET16 subroutine and returns to the
SWEET16 calling program which resumes execution (in SWEET16 mode). R12,
which is the SWEET16 subroutine return stack pointer, is decremented
twice.
Branch conditions are not changed.

### BS

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| BS ea | $0C | $rl | branch to SWEET16 subroutine |

A branch to the effective address (PC+2+d) is taken and execution is
resumed in SWEET16 mode. The current PC is pushed onto a "SWEET16
subroutine return address" stack whose pointer is R12, and R12 is
incremented by 2. The carry is cleared and branch conditions are set
to indicate the current ACC contents.

Example:
```
0300: 15 34 A0       SET   R5,$A034 ; init pointer 1
0303: 14 3B A0       SET   R4,$A03B ; init limit 1
0306: 16 00 30       SET   R6,$3000 ; init pointer 2
0309: 0C 15          BS    MOVE
[...]
0320: 45       MOVE  LD   @R5       ; move one
0321: 56             ST   @R6       ;    byte
0322: 24             LD    R4
0323: 05             CPR   R5       ; test if done
0324: 04 FA          BP    MOVE
0326 :0B             RS             ; return
```

### NP

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| NP ea | $0D | $xx | no operation, reading dummy argument |
| NP ea | $0E | $xx | no operation, reading dummy argument |
| NP ea | $0F | $xx | no operation, reading dummy argument |

These opcodes do nothing like the 6502 NOP instruction. Unlike that
instruction, it also reads the following byte, so they behave more like
a branch never taken.

However, these could be used for further expansions.


## Operation Code Summary

| Name | Opcode | Argument | Function |
| ---- | ------ | -------- | -------- |
| [RTN] | $00 | - | return to 6502 mode |
| [BR ea](#br) | $01 | $rl | branch always |
| [BNC ea](#bnc) | $02 | $rl | branch if no carry |
| [BC ea](#bc) | $03 | $rl | branch if carry set |
| [BP ea](#bp) | $04 | $rl | branch if plus |
| [BM ea](#bm) | $05 | $rl | branch if minus |
| [BZ ea](#bz) | $06 | $rl | branch if zero |
| [BNZ ea](#bnz) | $07 | $rl | branch if nonzero |
| [BM1 ea](#bm1) | $08 | $rl | branch if minus 1 |
| [BNM1 ea](#bnm1) | $09 | $rl | branch if not minus 1 |
| [BK ea](#bk) | $0A | $xx | break |
| [RS](#rs) | $0B | - | return from SWEET16 subroutine |
| [BS ea](#bs) | $0C | $rl | branch to SWEET16 subroutine |
| [NP ea](#np) | $0D | $xx | no operation, reading dummy argument |
| [NP ea](#np) | $0E | $xx | no operation, reading dummy argument |
| [NP ea](#np) | $0F | $xx | no operation, reading dummy argument |
| [SET Rn, Constant](#set-rn) | $1n | $lb $hb | Set |
| [LD Rn](#ld-rn) | $2n | - | Load |
| [ST Rn](#st-rn) | $3n | - | Store |
| [LD @Rn](#ld-rn_1) | $4n | - | Load indirect |
| [ST @Rn](#st-rn_1) | $5n | - | Store indirect |
| [LDD @Rn](#ldd-rn) | $6n | - | Load double byte indirect |
| [STD @Rn](#std-rn) | $7n | - | Store double byte indirect |
| [POP @Rn](#pop-rn) | $8n | - | Pop indirect |
| [STP @Rn](#stp-rn) | $8n | - | Store pop indirect |
| [ADD Rn](#add-rn) | $An | - | Add |
| [SUB Rn](#sub-rn) | $Bn | - | Subtract |
| [POPD @Rn](#popd-rn) | $Cn | - | Pop double indirect byte |
| [CPR Rn](#cpr-rn) | $Dn | - | Compare |
| [INR Rn](#inr-rn) | $En | - | Increment |
| [DCR Rn](#dcr-rn) | $Fn | - | Decrement |
