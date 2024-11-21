
.export     MNEM
.export     FMT1
.export     FMT2
.export     CHAR1
.export     CHAR2

;******************************************************************************
;* the routines and data used in this file are based upon:                    *
;******************************************************************************
;*                                                                            *
;*  Apple //c                                                                 *
;*  Video Firmware and                                                        *
;*  Monitor ROM Source                                                        *
;*                                                                            *
;*  COPYRIGHT 1977-1983 BY                                                    *
;*  APPLE COMPUTER, INC.                                                      *
;*                                                                            *
;*  ALL RIGHTS RESERVED                                                       *
;*                                                                            *
;*  S. WOZNIAK           1977                                                 *
;*  A. BAUM              1977                                                 *
;*  JOHN A           NOV 1978                                                 *
;*  R. AURICCHIO     SEP 1982                                                 *
;*  E. BEERNINK          1983                                                 *
;*                                                                            *
;******************************************************************************
;* however those routines were heavily modified to fit the Sorbus Computer    *
;******************************************************************************

.ifp02
.else
.error This table is for NMOS 6502 only
.endif

.segment "DATA"

;******************************************************************************
MNEM:
   ; format is:
   ; convert each letter to 5 bits by
   ; - ascii & 0x1f + 1 ($00 = ASCII of "?")
   ; word offsets:
   ; - 1st letter << 11
   ; - 2nd letter <<  6
   ; - 3rd letter <<  1
   .word $1CD8             ; $00:"BRK"
   .word $8A62             ; $01:"PHP"
   .word $1C5A             ; $02:"BPL"
   .word $2348             ; $03:"CLC"
   .word $5D26             ; $04:"JSR"
   .word $8B62             ; $05:"PLP"
   .word $1B94             ; $06:"BMI"
   .word $A188             ; $07:"SEC"
   .word $9D54             ; $08:"RTI"
   .word $8A44             ; $09:"PHA"
   .word $1DC8             ; $0A:"BVC"
   .word $2354             ; $0B:"CLI"
   .word $9D68             ; $0C:"RTS"
   .word $8B44             ; $0D:"PLA"
   .word $1DE8             ; $0E:"BVS"
   .word $A194             ; $0F:"SEI"
   .word $0000             ; $10:"???"
   .word $29B4             ; $11:"DEY"
   .word $1908             ; $12:"BCC"
   .word $AE84             ; $13:"TYA"
   .word $6974             ; $14:"LDY"
   .word $A8B4             ; $15:"TAY"
   .word $1928             ; $16:"BCS"
   .word $236E             ; $17:"CLV"
   .word $2474             ; $18:"CPY"
   .word $53F4             ; $19:"INY"
   .word $1BCC             ; $1A:"BNE"
   .word $234A             ; $1B:"CLD"
   .word $2472             ; $1C:"CPX"
   .word $53F2             ; $1D:"INX"
   .word $19A4             ; $1E:"BEQ"
   .word $A18A             ; $1F:"SED"
   .word $0000             ; $20:"???"
   .word $1AAA             ; $21:"BIT"
   .word $5BA2             ; $22:"JMP"
   .word $5BA2             ; $23:"JMP"
   .word $A574             ; $24:"STY"
   .word $6974             ; $25:"LDY"
   .word $2474             ; $26:"CPY"
   .word $2472             ; $27:"CPX"
   .word $AE44             ; $28:"TXA"
   .word $AE68             ; $29:"TXS"
   .word $A8B2             ; $2A:"TAX"
   .word $AD32             ; $2B:"TSX"
   .word $29B2             ; $2C:"DEX"
   .word $0000             ; $2D:"???"
   .word $7C22             ; $2E:"NOP"
   .word $0000             ; $2F:"???"
   .word $151A             ; $30:"ASL"
   .word $9C1A             ; $31:"ROL"
   .word $6D26             ; $32:"LSR"
   .word $9C26             ; $33:"ROR"
   .word $A572             ; $34:"STX"
   .word $6972             ; $35:"LDX"
   .word $2988             ; $36:"DEC"
   .word $53C8             ; $37:"INC"
   .word $84C4             ; $38:"ORA"
   .word $13CA             ; $39:"AND"
   .word $3426             ; $3A:"EOR"
   .word $1148             ; $3B:"ADC"
   .word $A544             ; $3C:"STA"
   .word $6944             ; $3D:"LDA"
   .word $23A2             ; $3E:"CMP"
   .word $A0C8             ; $3F:"SBC"

;******************************************************************************
; FMT1 BYTES:    XXXXXXY0 INSTRS
; IF Y=0         THEN RIGHT HALF BYTE
; IF Y=1         THEN LEFT HALF BYTE
;                   (X=INDEX)
;
FMT1:
   .byte $0F               ; $xF=BRK without argument (makes more sense on NMOS)
   .byte $22
   .byte $FF
   .byte $33
   .byte $CB
   .byte $62
   .byte $FF
   .byte $73
   .byte $03
   .byte $22
   .byte $FF
   .byte $33
   .byte $CB
   .byte $66
   .byte $FF
   .byte $77
   .byte $0F
   .byte $20
   .byte $FF
   .byte $33
   .byte $CB
   .byte $60
   .byte $FF
   .byte $70
   .byte $0F
   .byte $22
   .byte $FF
   .byte $39
   .byte $CB
   .byte $66
   .byte $FF
   .byte $7D
   .byte $0B
   .byte $22
   .byte $FF
   .byte $33
   .byte $CB
   .byte $A6
   .byte $FF
   .byte $73
   .byte $11
   .byte $22
   .byte $FF
   .byte $33
   .byte $CB
   .byte $A6
   .byte $FF
   .byte $87
   .byte $01
   .byte $22
   .byte $FF
   .byte $33
   .byte $CB
   .byte $60
   .byte $FF
   .byte $70
   .byte $01
   .byte $22
   .byte $FF
   .byte $33
   .byte $CB
   .byte $60
   .byte $FF
   .byte $70
   .byte $24
   .byte $31
   .byte $65
   .byte $78
; ZZXXXY01 INSTR'S
FMT2:
   .byte $00               ; $00: ERR
   .byte $21               ; $01: #IMM
   .byte $81               ; $02: Z-PAGE
   .byte $82               ; $03: ABS
   .byte $59               ; $04: (ZPAG,X)
   .byte $4D               ; $05: (ZPAG),Y
   .byte $91               ; $06: ZPAG,X
   .byte $92               ; $07: ABS,X
   .byte $86               ; $08: ABS,Y
   .byte $4A               ; $09: (ABS)
   .byte $85               ; $0a: ZPAG,Y
   .byte $9D               ; $0b: RELATIVE
   .byte $49               ; $0c: (ZPAG)      (new)
   .byte $5A               ; $0d: (ABS,X)     (new)
;
CHAR2:   ; table had originally bit 7 set on all non-zero values
   .byte $59               ;'Y'
   .byte $00               ; $0f: implied (of FMT2)
   .byte $58               ;'X'
   .byte $24               ;'$'
   .byte $24               ;'$'
   .byte $00
;
CHAR1:   ; table had originally bit 7 set on all values
   .byte $2C               ;','
   .byte $29               ;')'
   .byte $2C               ;','
   .byte $23               ;'#'
   .byte $28               ;'('
   .byte $24               ;'$'
