; Conway\'s Game of Life
; http://rosettacode.org/wiki/Conway\'s_Game_of_Life
; Submitted by Anonymous

.include "fb32x32.inc"

sorbusinit:
   lda   #$00
   ldx   #$cc
   ldy   #$01
   int   FB32X32

    sei
    lda #FB32X32_CMAP_C64
    sta FB32X32_COLMAP
    lda #<irqhandler
    sta UVNBI+0
    lda #>irqhandler
    sta UVNBI+1
    lda #$90
    sta TMIMRL
    lda #$01
    sta TMIMRH

    jsr inittick
    cli
:
    jsr CHRIN
    cmp #$03
    bne :-
    stz TMIMRL
    stz TMIMRH
    jmp ($fffc)

irqhandler:

 pha
 phx
 phy

 jsr starttick
 
 stz FB32X32_COPY
 lda TMIMRL
 ply
 plx
 pla
 rti

inittick:

randfill:   stz $11          ;$200 for indirect
            ldx #$cc         ;addressing
            stx $12
randloop:   lda RANDOM       ;generate random
            and #$01         ;pixels on the
            sta ($11),Y      ;screen
            jsr inc1113
            cmp #$00
            bne randloop
            lda $12
            cmp #$d0
            bne randloop
 
clearmem:   lda #$df         ;set $07df-$0a20
            sta $11          ;to $#00
            lda #$07
            sta $12
clearbyte:  lda #$00
            sta ($11),Y
            jsr inc1113
            cmp #$20
            bne clearbyte
            lda $12
            cmp #$0a
            bne clearbyte
            rts
 
starttick:
            inc $15
            lda $15
            cmp #$64
            bne copyscreen
            stz $15
            
            jsr inittick
copyscreen: lda #$00         ;set up source
            sta $11          ;pointer at
            sta $13          ;$01/$02 and
            lda #$cc         ;dest pointer
            sta $12          ;at $03/$04
            lda #$08
            sta $14
            ldy #$00
copybyte:   lda ($11),Y      ;copy pixel to
            sta ($13),Y      ;back buffer
            jsr inc1113      ;increment pointers
            cmp #$00         ;check to see
            bne copybyte     ;if we\'re at $600
            lda $12          ;if so, we\'ve
            cmp #$06         ;copied the
            bne copybyte     ;entire screen
 
 
conway:     lda #$df         ;apply conway rules
            sta $11          ;reset the pointer
            sta $13          ;to $#01df/$#07df
            lda #$cb         ;($cc00 - $21)
            sta $12          ;($800 - $21)
            lda #$07
            sta $14
onecell:    lda #$00         ;process one cell
            ldy #$01         ;upper cell
            clc
            adc ($13),Y
            ldy #$41         ;lower cell
            clc
            adc ($13),Y
chkleft:    tax              ;check to see
            lda $11          ;if we\'re at the
            and #$1f         ;left edge
            tay
            txa
            cpy #$1f
            beq rightcells
leftcells:  ldy #$00         ;upper-left cell
            clc
            adc ($13),Y
            ldy #$20         ;left cell
            clc
            adc ($13),Y
            ldy #$40         ;lower-left cell
            clc
            adc ($13),Y
chkright:   tax              ;check to see
            lda $11          ;if we\'re at the
            and #$1f         ;right edge
            tay
            txa
            cpy #$1e
            beq evaluate
rightcells: ldy #$02         ;upper-right cell
            clc
            adc ($13),Y
            ldy #$22         ;right cell
            clc
            adc ($13),Y
            ldy #$42         ;lower-right cell
            clc
            adc ($13),Y
evaluate:   ldx #$01         ;evaluate total
            ldy #$21         ;for current cell
            cmp #$03         ;3 = alive
            beq storex
            ldx #$00
            cmp #$02         ;2 = alive if
            bne storex       ;c = alive
            lda ($13),Y
            and #$01
            tax
storex:     txa              ;store to screen
            sta ($11),Y
            jsr inc1113      ;move to next cell
conwayloop: cmp #$e0         ;if not last cell,
            bne onecell      ;process next cell
            lda $12
            cmp #$cf
            bne onecell
            ;jmp starttick    ;run next tick
            rts

 
inc1113:    lda $11          ;increment $01
            cmp #$ff         ;and $03 as 16-bit
            bne onlyinc01    ;pointers
            inc $12
            inc $14
onlyinc01:  inc $11
            lda $11
            sta $13
            rts
