; rewrite (c) 2024 by SvOlli
; SPDX-License-Identifier: GPL-3.0-or-later

.segment "CODE"

   ; should start at $0000
   ; whole memory is just $20 (=32) bytes
   ; -> code also needs to be 32 bytes
start:
   ;ldx   #$00 ; removed due to reset vector now at irq
   clc
   ; $5c is evaluated by ($xxxx = address of is65816):
   ; 6502:     NOP $xxxx,X ("illegal" opcode)
   ; 65(S)C02: NOP #$xxxx (reserved)
   ; 65816:    JMP $38xxxx ($38 taken from SEC)
   ; 65CE02:   AUG #$38xxxx ($38 taken from SEC)
   .byte $5c
   .word is65816
   ; 6502 and 65(S)C02 continue here from $5c
   sec
   ; 65CE02 continues here from $5c
   bcc   is65CE02
   txa           ; A=$00
   ; $1a is evaluated by:
   ; 6502:   NOP ("illegal" opcode)
   ; 65C02:  INC
   .byte $1a
   bne   check65sc02  ; 6502: A=$00, 65(S)C02: A=$01
   ror
   bcs   is6502noror
   bcc   is6502
check65sc02:
   ; $97 is evaluated by:
   ; 65C02:  SMB1 $FF ; will set retval to $02 = 65C02
   ; 65SC02: NOP(reserved) : NOP(reserved, $FF)
   .byte $97
   .byte $FF

is65SC02:
   inx            ; X=$06
is6502noror:
   inx            ; X=$05
is65CE02:
   inx            ; X=$04
is65816:
   ; 65816 continues here from $5c
   inx            ; X=$03
is65C02:
   inx            ; X=$02 ; will be set using SMB1 $FF above
is6502:
   inx            ; X=$01
   stx   $ff      ; will stop CPU

   .byte $ea,$4c ; unused
reset:
   .word irq      ; reset vector, start of ram
irq:
   ldx   #$00     ; argument needs to be $00
   ;.byte $ea     ; dummy for vector
   ;.byte $00     ; use irqvec hi for return value
   ;slip through to start
