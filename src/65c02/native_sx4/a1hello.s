
.segment "CODE"

SINTAB   := $0200

start:
   ldy   #$00
   sty   $10
   sty   $11
   sty   $12
   sty   $13
   ldx   #$3f
@sinloop:
   clc
   lda   $10
   adc   $12
   sta   $12
   lda   $11
   adc   $13
   sta   $13

   sta   SINTAB+$c0,y
   sta   SINTAB+$80,x
   eor   #$1f
   sta   SINTAB+$40,y
   sta   SINTAB+$00,x

   lda   #$02
   adc   $10
   sta   $10
   bcc   :+
   inc   $11
:
   iny
   dex
   bpl   @sinloop

   lda   #<(@text-1)
   sta   $10
   lda   #>(@text-1)
   sta   $11
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
   jsr   echo
   dex
   bne   :-
@norepeat:
   cmp   #$02
   bne   @noeffect
   jsr   @sinus
   beq   @printloop
@noeffect:
   ora   #$80
   jsr   echo
:
   dex
   bne   :-
; routine always returns with n=0
   bpl   @printloop
@end:
   jmp   ($fffc)
@getnext:
   inc   $10
   bne   :+
   inc   $11
:
   lda   ($10),y
   rts
@sinus:
   ldy   #$10
@yloop:
   lda   SINTAB,y
   clc
   adc   #$04
   tax
   lda   #$a0
@xloop:
   jsr   echo
   dex
   bne   @xloop
   lda   #$aa
   jsr   echo
   lda   #$8d
   jsr   echo
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

echo:
   ; simulate the output of the Apple 1
   ; first waste about 1/60th of a second
   pha
   phy
   lda   #$0d
   ldy   #$00
:
   iny                  ; 256 loop take ~1280 cycles
   bne   :-
   dec                  ; 13 loops should do it
   bne   :-

   ply
   pla
   and   #$7f
   cmp   #$01           ; used as delay
   bne   :+
   rts
:
   cmp   #$0d           ; convert CR to LF
   bne   :+
   lda   #$0a
:
   jmp   $FF03
