
.include "jam_bios.inc"

; ping sound reconstructed from EX-DOS II / Disk Doctor

; taken from cc65/asminc/c64.inc:
; I/O: SID

SID             := $D400
SID2            := SID + $20

SID_S1Lo        := SID + $00
SID_S1Hi        := SID + $01
SID_PB1Lo       := SID + $02
SID_PB1Hi       := SID + $03
SID_Ctl1        := SID + $04
SID_AD1         := SID + $05
SID_SUR1        := SID + $06

SID_S2Lo        := SID + $07
SID_S2Hi        := SID + $08
SID_PB2Lo       := SID + $09
SID_PB2Hi       := SID + $0A
SID_Ctl2        := SID + $0B
SID_AD2         := SID + $0C
SID_SUR2        := SID + $0D

SID_S3Lo        := SID + $0E
SID_S3Hi        := SID + $0F
SID_PB3Lo       := SID + $10
SID_PB3Hi       := SID + $11
SID_Ctl3        := SID + $12
SID_AD3         := SID + $13
SID_SUR3        := SID + $14

SID_FltLo       := SID + $15
SID_FltHi       := SID + $16
SID_FltCtl      := SID + $17
SID_Amp         := SID + $18
SID_ADConv1     := SID + $19
SID_ADConv2     := SID + $1A
SID_Noise       := SID + $1B
SID_Read3       := SID + $1C

SID2_S1Lo        := SID2 + $00
SID2_S1Hi        := SID2 + $01
SID2_PB1Lo       := SID2 + $02
SID2_PB1Hi       := SID2 + $03
SID2_Ctl1        := SID2 + $04
SID2_AD1         := SID2 + $05
SID2_SUR1        := SID2 + $06

SID2_S2Lo        := SID2 + $07
SID2_S2Hi        := SID2 + $08
SID2_PB2Lo       := SID2 + $09
SID2_PB2Hi       := SID2 + $0A
SID2_Ctl2        := SID2 + $0B
SID2_AD2         := SID2 + $0C
SID2_SUR2        := SID2 + $0D

SID2_S3Lo        := SID2 + $0E
SID2_S3Hi        := SID2 + $0F
SID2_PB3Lo       := SID2 + $10
SID2_PB3Hi       := SID2 + $11
SID2_Ctl3        := SID2 + $12
SID2_AD3         := SID2 + $13
SID2_SUR3        := SID2 + $14

SID2_FltLo       := SID2 + $15
SID2_FltHi       := SID2 + $16
SID2_FltCtl      := SID2 + $17
SID2_Amp         := SID2 + $18
SID2_ADConv1     := SID2 + $19
SID2_ADConv2     := SID2 + $1A
SID2_Noise       := SID2 + $1B
SID2_Read3       := SID2 + $1C

.segment "DATA"

pingdata:
   .byte $bb,$22,$00,$00,$10,$0a,$00
   .byte $cf,$22,$00,$00,$10,$0a,$00
   .byte $e3,$22,$00,$00,$10,$0a,$00
   .byte $00,$00,$00,$0f
pingdataend:

.segment "CODE"

playping:
   ldx   #$18
:
   lda   pingdata,x
   sta   SID,x
   sta   SID2,x
   dex
   bpl   :-
   lda   #$11
   sta   SID_Ctl1
   sta   SID_Ctl2
   sta   SID_Ctl3
   sta   SID2_Ctl1
   sta   SID2_Ctl2
   sta   SID2_Ctl3
:
   lda   SID_Read3
   bpl   :-
:
   lda   SID_Read3
   bne   :-
   stz   SID_Ctl1
   stz   SID_Ctl2
   stz   SID_Ctl3
   stz   SID2_Ctl1
   stz   SID2_Ctl2
   stz   SID2_Ctl3

   jsr   PRINT
   .byte 10,"done. ",0

:
   jsr   CHRIN
   bcs   :-
   cmp   #$03
   bne   playping
   jmp   ($fffc)
