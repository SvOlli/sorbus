\ This file is licensed under the terms of the 2-clause BSD license. Please
\ see the COPYING file in the root project directory for the full text.

.include "cpm65.inc"

BANK   = $DF00
TRAP   = $DF01
IDBASE = $DF70
ID_LBA = IDBASE+$0 \ alternative notation: ID_LBA+0/ID_LBA+1
ID_MEM = IDBASE+$2 \ alternative notation: ID_MEM+0/ID_MEM+1
IDREAD = IDBASE+$4 \ (S) read sector (adjusts DMA memory and LBA)
                   \ (R) error code from last read ($00=ok)
IDWRT  = IDBASE+$5 \ (S) write sector (adjusts DMA memory and LBA)
                   \ (R) error code from last write ($00=ok)
IDTRIM = IDBASE+$7 \ sector discard (strobe)
BUFFER = $DF80
RESET  = $FFFC

.expand 1

\ --- Resident part starts at the top of the file ---------------------------

start:
   \ misusing cpm_fcb as first argument to parse
   lda cpm_fcb+1
   cmp   #'E'
   beq   reboot
   cmp   #'T'
   beq   trim

   lda   #<m_help
   ldx   #>m_help
   ldy   #BDOS_PRINTSTRING
   jmp   BDOS

check:
   ldx   #$00
resetreg:
   lda   TRAP
   beq   check2
   inx
   bne   resetreg
   beq   nosorbus
check2:
   lda   TRAP
   cmp   #'S'
   bne   nosorbus
   lda   TRAP
   cmp   #'B'
   bne   nosorbus
   lda   TRAP
   cmp   #'C'
   bne   nosorbus
resetreg2:
   lda   TRAP
   bne   resetreg2
   rts

nosorbus:
   lda   #<m_nosorbus
   ldx   #>m_nosorbus
   ldy   #BDOS_PRINTSTRING
   pla
   pla
   jmp   BDOS

reboot:
   jsr   check
   lda   #<m_reboot
   ldx   #>m_reboot
   ldy   #BDOS_PRINTSTRING
   jsr   BDOS

   lda   #$01
   sta   BANK
   jmp   (RESET)

trim:
   jsr   check
   lda   #$00
   sta   ID_LBA+0
   lda   #$01
   sta   ID_LBA+1

trimreadloop:
   lda   #$80
   sta   ID_MEM+0
   lda   #$DF
   sta   ID_MEM+1
   sta   IDREAD

   rts

m_help:
   .byte "Parameters: exit trim", 10, 0
m_nosorbus:
   .byte "This program only runs on Sorbus Computers", 10, 0
m_reboot:
   .byte "Rebooting into Sorbus kernel", 10, 0

   .bss freemap, 2048

\ vim: sw=4 ts=4 et

