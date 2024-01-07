
VEC    := $08
BANK   := $df00
EXIT   := $e000
CHKOUT := $ff03
PRINT  := $ff06
IOSTRT := $d0
IOEND  := $e0

   ldx   #$ff
   txs
   jsr   PRINT
   .byte 10,"memcheck",10,"writing",10,0
   ldx   #$01
   stx   BANK
   ldy   #$00
   ldx   #$05           ; start ram write at $0500
   sty   VEC+0
@writeloopx:
   stx   VEC+1
@writeloop:
   tya
   sta   (VEC),y
   iny
   txa
   sta   (VEC),y
   iny
   bne   @writeloop
   inx
   beq   @writedone
   cpx   #IOSTRT
   bne   :+
   ldx   #IOEND
:
   bne   @writeloopx
@writedone:

   jsr   PRINT
   .byte "reading",10,0
   stx   BANK
   ldx   #$05           ; start ram read at $0500
@readloopx:
   stx   VEC+1
@readloop:
   tya
   cmp   (VEC),y
   beq   :+
   jsr   reportaddr
:
   iny
   txa
   cmp   (VEC),y
   beq   :+
   jsr   reportaddr
:
   iny
   bne   @readloop
   inx
   beq   @readdone
   cpx   #IOSTRT
   bne   :+
   ldx   #IOEND
:
   bne   @readloopx
@readdone:
   inc   BANK
   jsr   PRINT
   .byte "done",10,0
   jmp   EXIT

reportaddr:
   inc   BANK
   jsr   hexout16
   stz   BANK
   rts

hexout16:
   txa
   jsr   hexout8
   tya
   jsr   hexout8
   lda   #' '
   jmp   CHKOUT

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
   jmp   CHKOUT
