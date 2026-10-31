
# RP2040 Assembly

At one point the decision was made to code some timing critical parts,
where the execution should be either as fast as possible, or very
predictable to the clockcycle, in assembly. These notes are intended to
help getting quick acces on what and what not is possible on the
RP2040's ARM Cortex M0+ cores. This document may contain simplifications
for the usage within the Sorbus Computer.

The full reference is:
[ARMv6-M Architecture Reference Manual](https://users.ece.utexas.edu/~valvano/mspm0/Arm_Architecture_v6m_Reference_Manual.pdf)

## Instruction Set Overview

Note that only 5 instructions allow the use of high registers:
ADD, BLX, BX, CMP, and MOV.

| Instruction | Cycles | Operand form / addressing mode |
| ----------- | ------ | ------------------------------ |
| ADCS  | 1 | Rdn, Rm |
| ADD*  | 1 | Rdn\*, Rm\* - high-register form; does not set flags |
| ADDS  | 1 | Rd, Rn, Rm |
| ADDS  | 1 | Rd, Rn, #imm3 |
| ADDS  | 1 | Rd, #imm8 |
| ADD   | 1 | Rd, SP, #imm8 - does not set flags |
| ADD   | 1 | SP, SP, #imm8 - does not set flags |
| ADR   | 1 | Rd, label - PC-relative address |
| ANDS  | 1 | Rd, Rn, Rm |
| ASRS  | 1 | Rd, Rm, #imm8 |
| ASRS  | 1 | Rd, Rn, Rm |
| B     | 2 | label - unconditional |
| BCC / BLO | 1/2** | label - aliases |
| BCS / BHS | 1/2** | label - aliases |
| BEQ   | 1/2** | label |
| BGE   | 1/2** | label |
| BGT   | 1/2** | label |
| BHI   | 1/2** | label |
| BLE   | 1/2** | label |
| BLS   | 1/2** | label |
| BLT   | 1/2** | label |
| BMI   | 1/2** | label |
| BNE   | 1/2** | label |
| BPL   | 1/2** | label |
| BVC   | 1/2** | label |
| BVS   | 1/2** | label |
| BICS  | 1 | Rd, Rn, Rm |
| BL    | 3 | label |
| BLX*  | 2 | Rm\* |
| BX*   | 2 | Rm\* |
| CMN   | 1 | Rn, Rm |
| CMP   | 1 | Rn, Rm |
| CMP   | 1 | Rn, #imm8 |
| CMP*  | 1 | Rn\*, Rm\* - high-register form |
| EORS  | 1 | Rd, Rn, Rm |
| IT    | 1 | cond, mask |
| LDM / LDMIA | 1+N | Rn!, \{registers} |
| LDR   | 1/2*** | Rt, [Rn] |
| LDR   | 1/2*** | Rt, [Rn, #imm5] |
| LDR   | 1/2*** | Rt, [Rn, Rm] |
| LDR   | 2 | Rt, [PC, #imm8] - literal load |
| LDR   | 2 | Rt, =constant (pseudo instruction) |
| LDR   | 1/2*** | Rt, [SP, #imm8] |
| LDRB  | 1/2*** | Rt, [Rn] |
| LDRB  | 1/2*** | Rt, [Rn, #imm5] |
| LDRB  | 1/2*** | Rt, [Rn, Rm] |
| LDRH  | 1/2*** | Rt, [Rn] |
| LDRH  | 1/2*** | Rt, [Rn, #imm5] |
| LDRH  | 1/2*** | Rt, [Rn, Rm] |
| LDRSB | 1/2*** | Rt, [Rn, Rm] |
| LDRSH | 1/2*** | Rt, [Rn, Rm] |
| LSLS  | 1 | Rd, Rm, #imm5 |
| LSLS  | 1 | Rd, Rn, Rm |
| LSRS  | 1 | Rd, Rm, #imm5 |
| LSRS  | 1 | Rd, Rn, Rm |
| MOV*  | 1 | Rd\*, Rm\* - high-register form; does not set flags |
| MOVS  | 1 | Rd, Rm |
| MOVS  | 1 | Rd, #imm8 |
| MULS  | 1 | Rd, Rn, Rm |
| MVNS  | 1 | Rd, Rm |
| ORRS  | 1 | Rd, Rn, Rm |
| POP   | 1+N**** | \{registers} |
| PUSH  | 1+N | \{registers} |
| REV   | 1 | Rd, Rm |
| REV16 | 1 | Rd, Rm |
| REVSH | 1 | Rd, Rm |
| RORS  | 1 | Rd, Rm |
| RSBS  | 1 | Rd, Rn, #0 |
| SBCS  | 1 | Rd, Rn, Rm |
| STM / STMIA | 1+N | Rn!, \{registers} |
| STR   | 1/2*** | Rt, [Rn] |
| STR   | 1/2*** | Rt, [Rn, #imm5] |
| STR   | 1/2*** | Rt, [Rn, Rm] |
| STR   | 1/2*** | Rt, [SP, #imm8] |
| STRB  | 1/2*** | Rt, [Rn] |
| STRB  | 1/2*** | Rt, [Rn, #imm5] |
| STRB  | 1/2*** | Rt, [Rn, Rm] |
| STRH  | 1/2*** | Rt, [Rn] |
| STRH  | 1/2*** | Rt, [Rn, #imm5] |
| STRH  | 1/2*** | Rt, [Rn, Rm] |
| SUBS  | 1 | Rd, Rn, Rm |
| SUBS  | 1 | Rd, Rn, #imm3 |
| SUBS  | 1 | Rd, #imm8 |
| SUB   | 1 | Rd, SP, #imm7 - does not set flags |
| SUB   | 1 | SP, SP, #imm7 - does not set flags |
| SXTB  | 1 | Rd, Rm |
| SXTH  | 1 | Rd, Rm |
| TST   | 1 | Rn, Rm |
| UXTB  | 1 | Rd, Rm |
| UXTH  | 1 | Rd, Rm |
| BKPT  | - | #imm |
| CPSID | 1 | i and/or f |
| CPSIE | 1 | i and/or f |
| DMB   | 1 | No operands |
| DSB   | 1 | No operands |
| ISB   | 1 | No operands |
| MRS   | 2 | Rd, special-register |
| MSR   | 2 | special-register, Rn |
| NOP   | 1 | No operands |
| SEV   | 1 | No operands |
| SVC   | - | #imm |
| WFE   | variable | No operands |
| WFI   | variable | No operands |
| YIELD | 1 | No operands |

- `N` means the number of registers transferred.
- `-` means there is no single normal completion time: the instructions
  effect or duration depends on debug, exception, or sleep/wakeup
  handling.
- `*` marks an operand that can use a high register, subject to restrictions on `SP` and `PC`.
- `**` 1 cycle branch not taken, 2 cycles branch taken.
- `***` 1 cycle to SIO; 2 cycles to SRAM, ROM, XIP flash (best case),
  and AHB (Advanced High-performance Bus) peripherals; 3 cycles for APB
  (Advanced Peripheral Bus) reads; 4 cycles for APB writes. XIP flash
  may take longer on a cache miss.
- `****` add 2 cycles if PC is included.
- `LDR Rt, =constant` is an assembler pseudo-instruction, not a hardware instruction.


## Short Explanations Of Instructions

| Instruction | What it does |
| ----------- | ------------ |
| ADCS  | Adds two values and the carry flag and updates the condition flags |
| ADD   | Adds two values |
| ADDS  | Adds two values and updates the condition flags |
| ADR   | Places a PC-relative address in a register |
| ANDS  | Performs a bitwise AND and updates the condition flags |
| ASRS  | Arithmetic-shifts a value right and updates the condition flags |
| B     | Branches to a label |
| BCC / BLO | Branches if carry is clear / lower |
| BCS / BHS | Branches if carry is set / higher or same |
| BEQ   | Branches if equal |
| BGE   | Branches if greater than or equal |
| BGT   | Branches if greater than |
| BHI   | Branches if higher |
| BLE   | Branches if less than or equal |
| BLS   | Branches if lower or same |
| BLT   | Branches if less than |
| BMI   | Branches if negative |
| BNE   | Branches if not equal |
| BPL   | Branches if positive or zero |
| BVC   | Branches if overflow is clear |
| BVS   | Branches if overflow is set |
| BICS  | Performs a bitwise AND with the complement of a value and updates the condition flags |
| BKPT  | Causes a breakpoint |
| BL    | Branches to a label and stores the return address |
| BLX   | Branches to an address in a register and stores the return address |
| BX    | Branches to an address in a register |
| CMN   | Compares by adding values and updates the condition flags |
| CMP   | Compares values by subtraction and updates the condition flags |
| CPSID | Disables interrupts |
| CPSIE | Enables interrupts |
| DMB   | Ensures memory accesses before it complete before those after it |
| DSB   | Waits for prior memory accesses to complete |
| EORS  | Performs a bitwise exclusive OR and updates the condition flags |
| ISB   | Ensures subsequent instructions are fetched after the instruction barrier |
| IT    | Applies conditional execution to following instructions |
| LDM / LDMIA | Loads multiple registers from memory |
| LDR   | Loads a word from memory |
| LDRB  | Loads an unsigned byte from memory |
| LDRH  | Loads an unsigned halfword from memory |
| LDRSB | Loads a signed byte from memory |
| LDRSH | Loads a signed halfword from memory |
| LSLS  | Logical-shifts a value left and updates the condition flags |
| LSRS  | Logical-shifts a value right and updates the condition flags |
| MOV   | Copies a value between registers |
| MOVS  | Copies a value and updates the condition flags |
| MRS   | Reads a special register into a general-purpose register |
| MSR   | Writes a general-purpose register to a special register |
| MULS  | Multiplies two values and updates the condition flags |
| MVNS  | Bitwise-inverts a value and updates the condition flags |
| NOP   | Performs no operation |
| ORRS  | Performs a bitwise OR and updates the condition flags |
| POP   | Loads registers from the stack |
| PUSH  | Stores registers on the stack |
| REV   | Reverses the byte order of a word |
| REV16 | Reverses the byte order in each halfword |
| REVSH | Reverses the bytes in a halfword and sign-extends the result |
| RORS  | Rotates a value right and updates the condition flags |
| RSBS  | Subtracts a value from zero and updates the condition flags |
| SBCS  | Subtracts a value and the inverse of the carry flag, then updates the condition flags |
| SEV   | Sends an event to the event mechanism |
| STM / STMIA | Stores multiple registers to memory |
| STR   | Stores a word to memory |
| STRB  | Stores a byte to memory |
| STRH  | Stores a halfword to memory |
| SUB   | Subtracts a value |
| SUBS  | Subtracts a value and updates the condition flags |
| SVC   | Makes a supervisor call |
| SXTB  | Sign-extends a byte to a word |
| SXTH  | Sign-extends a halfword to a word |
| TST   | Tests values with a bitwise AND and updates the condition flags |
| UXTB  | Zero-extends a byte to a word |
| UXTH  | Zero-extends a halfword to a word |
| WFE   | Waits for an event |
| WFI   | Waits for an interrupt |
| YIELD | Signals that the processor can yield execution |


## Assembler Directives

| Directive | Function |
| --------- | -------- |
| .align   | Pad the location counter to a particular storage boundary |
| .ascii   | Defines memory for an ASCII string with no NULL terminator |
| .asciz   | Defines memory for an ASCII string and adds a NULL terminator |
| .byte    | Defines memory for bytes |
| .data    | Assembles following code to the end of the data subsection |
| .double  | Defines memory for double-precision floating-point data |
| .dword   | Defines storage for 64-bit integers |
| .else    | Part of conditional assembly |
| .elseif  | Part of conditional assembly |
| .endif   | Part of conditional assembly |
| .endm    | End of a macro definition |
| .endr    | End of a repeat block |
| .equ     | Defines values for symbols |
| .fill    | Define and fill some memory |
| .float   | Define memory for single-precision floating-point data |
| .global  | Make a symbol global, needed if reference from other files |
| .hword   | Defines memory for 16-bit integers |
| .if      | Marks the beginning of code to be conditionally assembled |
| .include | Merge a file into the current file |
| .int     | Define storage for 32-bit integers |
| .long    | Define storage for 32-bit integers (same as .int) |
| .macro   | Define a macro |
| .octa    | Defines storage for 64-bit integers |
| .quad    | Same as .octa |
| .rept    | Repeat a block of code multiple times |
| .set     | Set the value of a symbol to an expression |
| .short   | Same as .hword |
| .single  | Same as .float |
| .text    | Generate following instructions into the code section |
| .word    | Same as .int |
