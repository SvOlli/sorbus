
.include "native_bios.inc"

vector := $10
sign   := $12

; testing BCD addition by using a table for verification

   jmp   main

hexout8:
   pha
   lsr
   lsr
   lsr
   lsr
   jsr   hexdigit
   pla
   and   #$0f
hexdigit:
   ora   #'0'
   cmp   #':'
   bcc   :+
   adc   #$06
:
   jmp   CHROUT

printbad:
   pha
   php
   lda   dectab,x
   jsr   hexout8
   lda   sign
   jsr   CHROUT
   lda   dectab,y
   jsr   hexout8
   lda   sign
   jsr   CHROUT
   lda   #$00
   plp
   rol
   ora   #'0'
   jsr   CHROUT
   lda   #'='
   jsr   CHROUT
   pla
   jsr   hexout8
   lda   #$0a
   jmp   CHROUT


main:
   ldx   #$ff
   txs

   lda   #'+'
   sta   sign

   jsr   PRINT
   .byte 10,"ADC test, carry clear",10,10,0

   ldx   #$00
   ldy   #$00
   lda   #<(inctab+$00)
   sta   vector+0
   lda   #>(inctab+$00)
   sta   vector+1

adccloop:
   sed
   clc
   lda   inctab,x
   adc   inctab,y
   cld

   cmp   (vector),y
   beq   @okay
   
   clc
   jsr   printbad

@okay:
   iny
   cpy   #$64
   bcc   adccloop
   inc   vector+0

   ldy   #$00
   inx
   cpx   #$64
   bcc   adccloop
   
   jsr   PRINT
   .byte 10,"ADC test, carry set",10,10,0

   ldx   #$00
   ldy   #$00
   lda   #<(inctab+$01)
   sta   vector+0
   lda   #>(inctab+$01)
   sta   vector+1

adcsloop:
   sed
   sec
   lda   inctab,x
   adc   inctab,y
   cld

   cmp   (vector),y
   beq   @okay
   
   sec
   jsr   printbad

@okay:
   iny
   cpy   #$64
   bcc   adcsloop
   inc   vector+0

   ldy   #$00
   inx
   cpx   #$64
   bcc   adcsloop

   lda   #'-'
   sta   sign

   jsr   PRINT
   .byte 10,"SBC test, carry set",10,10,0

   ldx   #$00
   ldy   #$00
   lda   #<(inctab+$64)
   sta   vector+0
   lda   #>(inctab+$64)
   sta   vector+1
   
sbcsloop:
   sed
   sec
   lda   dectab,x
   sbc   dectab,y
   cld

   cmp   (vector),y
   beq   @okay
   
   clc
   jsr   printbad

@okay:
   iny
   cpy   #$64
   bcc   sbcsloop
   dec   vector+0

   ldy   #$00
   inx
   cpx   #$64
   bcc   sbcsloop

   jsr   PRINT
   .byte 10,"with carry clear",10,10,0

   ldx   #$00
   ldy   #$00
   lda   #<(inctab+$63)
   sta   vector+0
   lda   #>(inctab+$63)
   sta   vector+1

sbccloop:
   sed
   clc
   lda   dectab,x
   sbc   dectab,y
   cld

   cmp   (vector),y
   beq   @okay
   
   sec
   jsr   printbad

@okay:
   iny
   cpy   #$64
   bcc   sbccloop
   dec   vector+0

   ldy   #$00
   inx
   cpx   #$64
   bcc   sbccloop

   jsr   PRINT
   .byte 10,"done",10,0
   jmp   ($fffc)

inctab:
   .byte $00,$01,$02,$03,$04,$05,$06,$07,$08,$09
   .byte $10,$11,$12,$13,$14,$15,$16,$17,$18,$19
   .byte $20,$21,$22,$23,$24,$25,$26,$27,$28,$29
   .byte $30,$31,$32,$33,$34,$35,$36,$37,$38,$39
   .byte $40,$41,$42,$43,$44,$45,$46,$47,$48,$49
   .byte $50,$51,$52,$53,$54,$55,$56,$57,$58,$59
   .byte $60,$61,$62,$63,$64,$65,$66,$67,$68,$69
   .byte $70,$71,$72,$73,$74,$75,$76,$77,$78,$79
   .byte $80,$81,$82,$83,$84,$85,$86,$87,$88,$89
   .byte $90,$91,$92,$93,$94,$95,$96,$97,$98,$99
   .byte $00,$01,$02,$03,$04,$05,$06,$07,$08,$09
   .byte $10,$11,$12,$13,$14,$15,$16,$17,$18,$19
   .byte $20,$21,$22,$23,$24,$25,$26,$27,$28,$29
   .byte $30,$31,$32,$33,$34,$35,$36,$37,$38,$39
   .byte $40,$41,$42,$43,$44,$45,$46,$47,$48,$49
   .byte $50,$51,$52,$53,$54,$55,$56,$57,$58,$59
   .byte $60,$61,$62,$63,$64,$65,$66,$67,$68,$69
   .byte $70,$71,$72,$73,$74,$75,$76,$77,$78,$79
   .byte $80,$81,$82,$83,$84,$85,$86,$87,$88,$89
   .byte $90,$91,$92,$93,$94,$95,$96,$97,$98,$99
dectab:
   .byte $99,$98,$97,$96,$95,$94,$93,$92,$91,$90
   .byte $89,$88,$87,$86,$85,$84,$83,$82,$81,$80
   .byte $79,$78,$77,$76,$75,$74,$73,$72,$71,$70
   .byte $69,$68,$67,$66,$65,$64,$63,$62,$61,$60
   .byte $59,$58,$57,$56,$55,$54,$53,$52,$51,$50
   .byte $49,$48,$47,$46,$45,$44,$43,$42,$41,$40
   .byte $39,$38,$37,$36,$35,$34,$33,$32,$31,$30
   .byte $29,$28,$27,$26,$25,$24,$23,$22,$21,$20
   .byte $19,$18,$17,$16,$15,$14,$13,$12,$11,$10
   .byte $09,$08,$07,$06,$05,$04,$03,$02,$01,$00
   .byte $99,$98,$97,$96,$95,$94,$93,$92,$91,$90
   .byte $89,$88,$87,$86,$85,$84,$83,$82,$81,$80
   .byte $79,$78,$77,$76,$75,$74,$73,$72,$71,$70
   .byte $69,$68,$67,$66,$65,$64,$63,$62,$61,$60
   .byte $59,$58,$57,$56,$55,$54,$53,$52,$51,$50
   .byte $49,$48,$47,$46,$45,$44,$43,$42,$41,$40
   .byte $39,$38,$37,$36,$35,$34,$33,$32,$31,$30
   .byte $29,$28,$27,$26,$25,$24,$23,$22,$21,$20
   .byte $19,$18,$17,$16,$15,$14,$13,$12,$11,$10
   .byte $09,$08,$07,$06,$05,$04,$03,$02,$01,$00
