
.export     MNEM
.export     FMT1
.export     FMT2
.export     CHAR1
.export     CHAR2

;******************************************************************************
;* the data used in this file are based upon:                                 *
;******************************************************************************
;*                                                                            *
;*  Apple II                                                                  *
;*  System Monitor                                                            *
;*                                                                            *
;*  Copyright 1977 by Apple Computer, Inc.                                    *
;*  All Rights Reserved                                                       *
;*                                                                            *
;*  S. Wozniak                                                                *
;*  A. Baum                                                                   *
;*                                                                            *
;******************************************************************************
;* however the data was significantly modified to fit the Sorbus Computer     *
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

FMT1:
   .byte $04
   .byte $20
   .byte $54
   .byte $30
   .byte $0d
   .byte $80
   .byte $04
   .byte $90
   .byte $03
   .byte $22
   .byte $54
   .byte $33
   .byte $0d
   .byte $80
   .byte $04
   .byte $90
   .byte $04
   .byte $20
   .byte $54
   .byte $33
   .byte $0d
   .byte $80
   .byte $04
   .byte $90
   .byte $04
   .byte $20
   .byte $54
   .byte $3b
   .byte $0d
   .byte $80
   .byte $04
   .byte $90
   .byte $00
   .byte $22
   .byte $44
   .byte $33
   .byte $0d
   .byte $c8
   .byte $44
   .byte $00
   .byte $11
   .byte $22
   .byte $44
   .byte $33
   .byte $0d
   .byte $c8
   .byte $44
   .byte $a9
   .byte $01
   .byte $22
   .byte $44
   .byte $33
   .byte $0d
   .byte $80
   .byte $04
   .byte $90
   .byte $01
   .byte $22
   .byte $44
   .byte $33
   .byte $0d
   .byte $80
   .byte $04
   .byte $90
   .byte $26
   .byte $31
   .byte $87
   .byte $9a
; ZZXXXY01 instr's
FMT2:
   .byte $00             ;err
   .byte $21             ;imm
   .byte $81             ;z-page
   .byte $82             ;abs
   .byte $00             ;implied
   .byte $00             ;accumulator
   .byte $59             ;(zpag,x)
   .byte $4d             ;(zpag),y
   .byte $91             ;zpag,x
   .byte $92             ;abs,x
   .byte $86             ;abs,y
   .byte $4a             ;(abs)
   .byte $85             ;zpag,y
   .byte $9d             ;relative
CHAR1:
   .byte ","
   .byte ")"
   .byte ","
   .byte "#"
   .byte "("
   .byte "$"
CHAR2:
   .byte "Y"
   .byte $00
   .byte "X"
   .byte "$"
   .byte "$"
   .byte $00
