
.segment "CODE"
start:
   ldy   #$00
   sty   $00
   sty   $01
   sty   $02
   sty   $03
   ldx   #$3f
@sinloop:
   clc
   lda   $00
   adc   $02
   sta   $02
   lda   $01
   adc   $03
   sta   $03

   sta   $07c0,y
   sta   $0780,x
   eor   #$1f
   sta   $0740,y
   sta   $0700,x

   lda   #$02
   adc   $00
   sta   $00
   bcc   :+
   inc   $01
:
   iny
   dex
   bpl   @sinloop

   lda   #<(@text-1)
   sta   $00
   lda   #>(@text-1)
   sta   $01
   ldy   #$00
@printloop:
   jsr   @getnext
   beq   @end
   cmp   #$01         ; rle
   bne   @norepeat
   jsr   @getnext
   tax                ; repeats
   jsr   @getnext
   ora   #$80
:
   jsr   $ffef
   dex
   bne   :-
@norepeat:
   cmp   #$02
   bne   @noeffect
   jsr   @sinus
   beq   @printloop
@noeffect:
   ora   #$80
   jsr   $ffef
:
   dex
   bne   :-
; routine always returns with n=0
   bpl   @printloop
@end:
   jmp   $ff00
@getnext:
   inc   $00
   bne   :+
   inc   $01
:
   lda   ($00),y
   rts
@sinus:
   ldy   #$00
@yloop:
   lda   $0700,y
   clc
   adc   #$04
   tax
   lda   #$a0
@xloop:
   jsr   $ffef
   dex
   bne   @xloop
   lda   #$aa
   jsr   $ffef
   lda   #$8d
   jsr   $ffef
   iny
   iny
   bne   @yloop
   rts
@text:
   .byte 1,25,13
   .byte $22,"AND TECHNICALLY...",13
   .byte " THIS IS A FIRST RELEASE",$22,13
   .byte "  -- SPOTTER OF TRSI",13
   .byte 1,120,1
   .byte 1,25,13
   .byte 13
   .byte "        __________  _____ ____",13
   .byte "       /_  __/ __ \/ ___//  _/",13
   .byte "        / / / /_/ /\__ \ / /  ",13
   .byte "       / / / _, _/___/ // /   ",13
   .byte "      /_/ /_/ \_\/____/___/   ",13
   .byte 13
   .byte "TRISTAR AND RED SECTOR INCORPORATED",13
   .byte 1,60,1
   .byte "              PRESENT",13
   .byte 1,60,1
   .byte 13
   .byte "     __  __ _ __ A __ _ __  __",13
   .byte "     \ \/ //_\\ \_/ //_\\ \/ /",13
   .byte "      >  </ _ \\   // _ \>  < ",13
   .byte "     /_/\__/ \_\[_]/_/ \__/\_\",13
   .byte 13
   .byte "              PRODUCT",13
   .byte 1,60,1
   .byte 13
   .byte "        THE SORBUS COMPUTER",13
   .byte 1,60,1
   .byte 13
   .byte "SINCE THIS IS THE REPLICA OF AN APPLE 1",13
   .byte "THERE'S NOT MUCH MORE ONE CAN DO THAN",13
   .byte "TO PRINT OUT TEXT LIKE A LINEPRINTER",13
   .byte 1,60,1
   .byte 13
   .byte "SO WELCOME TO A SYSTEM FROM 1976 THAT",13
   .byte "ONLY KNOWS ONE DIRECTION:",13
   .byte 1,60,1
   .byte "VORWAERTS IMMER, RUECKWAERTS NIMMER!",13
   .byte 13
   .byte "SO THERE WILL BE ONLY THE GREETINGS.",13
   .byte 13
   .byte "TRSI AND XAYAX SEND GREETINGS TO:",13
   .byte "- ABYSS CONNECTION",13
   .byte 1,24,1
   .byte "- AKRONYME ANALOGIKER",13
   .byte 1,21,1
   .byte "- ALPHA FLIGHT",13
   .byte 1,28,1
   .byte "- ATTENTION WHORE",13
   .byte 1,25,1
   .byte "- BAUKNECHT",13
   .byte 1,31,1
   .byte "- BRAIN CONTROL",13
   .byte 1,27,1
   .byte "- CENSOR DESIGN",13
   .byte 1,27,1
   .byte "- COCOON",13
   .byte 1,34,1
   .byte "- CREST",13
   .byte 1,35,1
   .byte "- DIGITAL DEMOLITION CREW",13
   .byte 1,17,1
   .byte "- FAIRLIGHT",13
   .byte 1,31,1
   .byte "- FULCRUM",13
   .byte 1,33,1
   .byte "- GASMAN",13
   .byte 1,34,1
   .byte "- GASPODE",13
   .byte 1,33,1
   .byte "- GENESIS*PROJECT",13
   .byte 1,25,1
   .byte "- GHOSTOWN",13
   .byte 1,32,1
   .byte "- HAUJOBB",13
   .byte 1,33,1
   .byte "- JAC!",13
   .byte 1,36,1
   .byte "- K2",13
   .byte 1,38,1
   .byte "- LFT",13
   .byte 1,37,1
   .byte "- LOONIES",13
   .byte 1,33,1
   .byte "- MEGA",13
   .byte 1,36,1
   .byte "- MEGAONE",13
   .byte 1,33,1
   .byte "- METALVOTZE",13
   .byte 1,30,1
   .byte "- NUANCE",13
   .byte 1,34,1
   .byte "- ONSLAUGHT",13
   .byte 1,31,1
   .byte "- PERFORMERS",13
   .byte 1,30,1
   .byte "- PLUSH",13
   .byte 1,35,1
   .byte "- RABENAUGE",13
   .byte 1,31,1
   .byte "- RETROGURU",13
   .byte 1,31,1
   .byte "- RGCD",13
   .byte 1,36,1
   .byte "- RTIFICIAL",13
   .byte 1,31,1
   .byte "- SCHNAPPSGIRLS",13
   .byte 1,27,1
   .byte "- SPECKDRUMM",13
   .byte 1,30,1
   .byte "- THE DREAMS",13
   .byte 1,30,1
   .byte "- THE SOLARIS AGENCY",13
   .byte 1,22,1
   .byte "- THE SOLUTION",13
   .byte 1,28,1
   .byte "- TITAN",13
   .byte 1,35,1
   .byte "- TLBR",13
   .byte 1,36,1
   .byte "- VANTAGE",13
   .byte 1,33,1
   .byte 13
   .byte 1,60,1,13
   .byte 13
   .byte "OKAY, SO LET'S TRY AT LEAST ONE EFFECT:",13
   .byte "A SCROLLING SINE",13
   .byte "(NOT TO BE MISTAKEN FOR A SINE SCROLLER)",13
   .byte 1,60,1
   .byte 13
   .byte 2
   .byte "                   * PHEW, FINALLY DONE!",13
   .byte 13
   .byte "MORE ABOUT THE",13
   .byte "- THE HARDWARE",13
   .byte "- THE SOFTWARE",13
   .byte "- AND THE ",$22,"DEMO",$22,"",13
   .byte "AT:",13
   .byte "--> HTTPS://XAYAX.NET/SORBUS",13
   .byte "--> HTTPS://GITHUB.COM/SVOLLI/SORBUS",13
   .byte 13
   .byte "MUSIC BY SKYRUNNER",13
   .byte "--> HTTPS://SOUNDCLOUD.COM/SKYRUNNER_BC/",13
   .byte "(... NOT ON DEVICE, OBVIOUSLY ...)",13
   .byte 13,0
