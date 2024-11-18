
.export     hexenter
.export     hexupdown
.export     memorydump
.export     prhex4
.export     prhex8
.export     prhex8s
.export     inbufhex4
.export     inbufhex8
.export     inbufsp
.export     inbufa

.import     CHROUT
.import     PRINT
.import     asc2hex
.import     getaddr
.import     getbyte
.import     getfirstnonspace
.import     prtsp

.import     INBUF
.import     TRAP
.importzp   MODE
.importzp   ADDR0
.importzp   ADDR1
.importzp   ADDR2
.importzp   PSAVE


.segment "CODE"

memorydump:
   jsr   getaddr
   bcs   @next
   lda   ADDR0+0
   sta   ADDR1+0
   lda   ADDR0+1
   sta   ADDR1+1
   jsr   getaddr
   bcs   memorydumpline

@addrloop:
   jsr   memorydumpline

   lda   #$10
   clc
   adc   ADDR1+0
   sta   ADDR1+0
   bcc   :+
   inc   ADDR1+1
   ; high byte flipped...
   bne   :+
   ; ...to zero: check if address is <= $000F (end of memory)
   lda   ADDR1+0
   cmp   #$0f
   bcc   @done
:
   ; now check if end has been reached or surpassed
   lda   ADDR1+1
   cmp   ADDR0+1
   bcc   @addrloop
   beq   @checklower
   bcs   @done
@checklower:
   lda   ADDR1+0
   cmp   ADDR0+0
   bcc   @addrloop

@done:
   rts

@next:
   lda   ADDR1+0
   clc
   adc   #$10
   sta   ADDR1+0
   bcc   :+
   inc   ADDR1+1
:
   ;slip through

memorydumpline:
   jsr   PRINT
   .byte 10," :",0

   lda   ADDR1+1
   sta   ADDR2+1
   jsr   prhex8
   lda   ADDR1+0
   sta   ADDR2+0
   jsr   prhex8
   jsr   prtsp

   ldy   #$00
@dataloop:
   cpy   #$08
   bne   :+
   jsr   prtsp
:
   jsr   prtsp

   lda   (ADDR1),y
   jsr   prhex8

   iny
   cpy   #$10
   bcc   @dataloop

@ascii:
   jsr   PRINT
   .byte "  ",0
   ldy   #$00
@asciiloop:
   lda   (ADDR1),y
   cmp   #' '
   bcc   :+
   cmp   #$7f
   bcs   :+
   .byte $2c
:
   lda   #'.'
   jsr   CHROUT
   iny
   cpy   #$10
   bcc   @asciiloop
   lda   #':'
   sta   MODE
   rts

prhex8s:
   pha
   jsr   prhex8
   pla
   rts

prhex8:
   pha
   lsr
   lsr
   lsr
   lsr
   jsr   prhex4
   pla
prhex4:
   and   #$0f
   ora   #'0'
   cmp   #':'
   bcc   :+
   adc   #$06
:
   jmp   CHROUT


updownaddr:
   lda   INBUF
   bne   @withdata
@lastprinted:
   lda   ADDR2+1
   sta   ADDR1+1
   lda   ADDR2+0
   sta   ADDR1+0    ; A now contains lowbyte of address
   clc              ; indicate last printed
   rts
@withdata:
   ldx   #$00
:
   inx
   lda   INBUF,x
   beq   @lastprinted
   cmp   #' '
   beq   :-
   jsr   asc2hex
   bmi   @lastprinted
   jsr   getaddr
.if 0
   bcc   :+
   ;sta   TRAP
:
.endif
   lda   ADDR0+1
   sta   ADDR1+1
   lda   ADDR0+0
   sta   ADDR1+0    ; A now contains lowbyte of address
   sec              ; indicate moved
   rts

hexenter:
   jsr   getfirstnonspace
   cmp   #':'
   bne   @done

   inx                  ; X still on ':'
   jsr   getaddr
   bcs   @fail

   sta   ADDR2+0
   sty   ADDR2+1
   ldy   #$00
:
   lda   INBUF,x
   beq   @done
   jsr   getbyte
   bcs   @fail
   sta   (ADDR0),y
   iny
   bne   :-

@done:
   clc
@fail:
   rts

hexupdown:
   sty   PSAVE          ; PSAVE = $c1 (up), $c2 (down)
   jsr   hexenter
   bcc   :+
   rts
:
   jsr   updownaddr
   bcc   @updownsetup
   lsr   PSAVE          ; PSAVE=$60 C=1 (up), PSAVE=$61 C=0 (down)
   bcs   @up
   ;clc
   adc   #$10
   sta   ADDR1+0
   bcc   :+
   inc   ADDR1+1
:
.ifp02
   jmp   @updownsetup
.else
   bra   @updownsetup
.endif
@up:
   ;sec
   sbc   #$10
   sta   ADDR1+0
   bcs   :+
   dec   ADDR1+1
:
   
@updownsetup:
   ldx   #$00

   lda   #':'
   jsr   inbufa
   lda   ADDR1+1
   jsr   inbufhex8
   lda   ADDR1+0
   jsr   inbufhex8

   ldy   #$00
:
   lda   #' '
   jsr   inbufsp
   lda   (ADDR1),y
   jsr   inbufhex8
   iny
   cpy   #$10
   bcc   :-
   rts
   
inbufhex8:
   pha
   lsr
   lsr
   lsr
   lsr
   jsr   inbufhex4
   pla
inbufhex4:
   and   #$0f
   ora   #'0'
   cmp   #':'
   bcc   inbufa
   adc   #$06
   .byte $2c            ; skip next
inbufsp:
   lda   #' '
inbufa:
   sta   INBUF,x
   inx
   rts
