
.export     newopcode
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
;*  Monitor ROM Source                                                        *
;*                                                                            *
;*  Copyright 1977-1983 by Apple Computer, Inc.                               *
;*  All Rights Reserved                                                       *
;*                                                                            *
;*  S. Wozniak           1977                                                 *
;*  A. Baum              1977                                                 *
;*  John A           NOV 1978                                                 *
;*  R. Auricchio     SEP 1982                                                 *
;*  E. Beernink          1983                                                 *
;*                                                                            *
;******************************************************************************
;* however those routines were heavily modified to fit the Sorbus Computer    *
;******************************************************************************

.segment "CODE"

;******************************************************************************
; NEWOPS translates the opcode in the Y register
; to a mnemonic table index and returns with Z=1.
; If Y is not a new opcode, Z=0.
;
newopcode:
   tya                     ;get the opcode
   ldx   #<(INDX-OPTBL-1)  ;check through new opcodes
:
   cmp   OPTBL,x           ;does it match?
   beq   :+                ;=>yes, get new index
   dex
   bpl   :-                ;else check next one
   rts                     ;not found, exit with BNE

:
   lda   INDX,x            ;lookup index for mnemonic
   ldy   #0                ;exit with BEQ
   rts

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
   .word $1CC4             ; $10:"BRA"
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
   .word $AD06             ; $20:"TSB"
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
   .word $8A72             ; $2D:"PHX"
   .word $7C22             ; $2E:"NOP"
   .word $8B72             ; $2F:"PLX"
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
   .word $A576             ; $40:"STZ"
   .word $ACC6             ; $41:"TRB"
   .word $8A74             ; $42:"PHY"
   .word $8B74             ; $43:"PLY"
   .word $0000             ; $44:"???" (was $FC)

;******************************************************************************
; FMT1 BYTES:    XXXXXXY0 INSTRS
; IF Y=0         THEN RIGHT HALF BYTE
; IF Y=1         THEN LEFT HALF BYTE
;                   (X=INDEX)
;
FMT1:
   .byte $01               ; was $0F -> now BRK is BRK #$00
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
; ZZXXXY01 instr's
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

;
; OPTBL is a table containing the new opcodes that
; wouldn't fit into the existing lookup table.
;
OPTBL:
   .byte $12               ;ORA (ZPAG)
   .byte $14               ;TRB ZPAG
   .byte $1A               ;INC A
   .byte $1C               ;TRB ABS
   .byte $32               ;AND (ZPAG)
   .byte $34               ;BIT ZPAG,X
   .byte $3A               ;DEC A
   .byte $3C               ;BIT ABS,X
   .byte $52               ;EOR (ZPAG)
   .byte $5A               ;PHY
   .byte $64               ;STZ ZPAG
   .byte $72               ;ADC (ZPAG)
   .byte $74               ;STZ ZPAG,X
   .byte $7A               ;PLY
   .byte $7C               ;JMP (ABS,X)
   .byte $89               ;BIT #IMM
   .byte $92               ;STA (ZPAG)
   .byte $9C               ;STZ ABS
   .byte $9E               ;STZ ABS,X
   .byte $B2               ;LDA (ZPAG)
   .byte $D2               ;CMP (ZPAG)
   .byte $F2               ;SBC (ZPAG)
   .byte $FC               ;??? (the unknown opcode)
;
; INDX contains pointers to the mnemonics for each of
; the opcodes in OPTBL.  Pointers with BIT 7
; set indicate extensions to MNEML or MNEMR.
;
INDX:
   .byte $38               ;ORA (ZPAG)
   .byte $41               ;TRB ZPAG
   .byte $37               ;INC A
   .byte $41               ;TRB ABS
   .byte $39               ;AND (ZPAG)
   .byte $21               ;BIT ZPAG,X
   .byte $36               ;DEC A
   .byte $21               ;BIT ABS,X
   .byte $3A               ;EOR (ZPAG)
   .byte $42               ;PHY
   .byte $40               ;STZ ZPAG
   .byte $3B               ;ADC (ZPAG)
   .byte $40               ;STZ ZPAG,X
   .byte $43               ;PLY
   .byte $22               ;JMP (ABS,X)
   .byte $21               ;BIT IMM
   .byte $3C               ;STA (ZPAG)
   .byte $40               ;STZ ABS
   .byte $40               ;STZ ABS,X
   .byte $3D               ;LDA (ZPAG)
   .byte $3E               ;CMP (ZPAG)
   .byte $3F               ;SBC (ZPAG)
   .byte $44               ;???
