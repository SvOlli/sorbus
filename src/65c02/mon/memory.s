
.include "../native_bios.inc"

.export     compare
.export     fill
.export     hunt
.export     transfer

.import     getaddr
.import     getbyte
.import     prhex8
.import     prterr
.import     prtnl
.import     prtsp
.import     skipspace

.import     INBUF

.importzp   MODE
.importzp   ADDR0
.importzp   ADDR1
.importzp   ADDR2
.importzp   FORMAT
.importzp   LENGTH

jmpcmd   := $0280
huntdata := jmpcmd+3

.segment "CODE"

compare:
   lda   #<docompare
   ldy   #>docompare
   jsr   getrange
   jsr   getaddr
   bcs   error
   jsr   prtnl          ; always returns N=0
   bpl   runloop        ; always true

fill:
   lda   #<dofill
   ldy   #>dofill
   jsr   getrange
   jsr   getbyte
   bcs   error
   sta   FORMAT
   bcc   runloop        ; always true, C=0

hunt:
   lda   #<dohunt
   ldy   #>dohunt
   jsr   getrange
   ldy   #$ff
:
   jsr   skipspace
   cmp   #$00
   beq   :+
   jsr   getbyte
   bcs   error
   iny
   sta   huntdata,y
   sty   LENGTH
   bpl   :-             ; always true
:
   jsr   prtnl
   bpl   runloop        ; always true

transfer:
   lda   #<dotransfer
   ldy   #>dotransfer
   jsr   getrange
   jsr   getaddr
   bcs   error
   ;bcc   runloop       ; slip through

runloop:
.ifp02
   ldy   #$00
   lda   (ADDR1),y
.else
   lda   (ADDR1)
.endif
   jsr   jmpcmd
   inc   ADDR0+0
   bne   :+
   inc   ADDR0+1
:
   inc   ADDR1+0
   bne   :+
   inc   ADDR1+1
:
   ldy   ADDR1+0
   cpy   ADDR2+0
   bne   runloop
   ldy   ADDR1+1
   cpy   ADDR2+1
   bne   runloop
   lda   INBUF
   sta   MODE
   rts

error:
   jmp   prterr

docompare:
.ifp02
   cmp   (ADDR0),y
.else
   cmp   (ADDR0)
.endif
   bne   praddr1
   rts

dofill:
   lda   FORMAT
.ifp02
   sta   (ADDR1),y
.else
   sta   (ADDR1)
.endif
rts0:
   rts

dohunt:
   ldy   LENGTH
:
   lda   (ADDR1),y
   cmp   huntdata,y
   bne   rts0
   dey
   bpl   :-
   ; slip through
praddr1:
   lda   ADDR1+1
   jsr   prhex8
   lda   ADDR1+0
   jsr   prhex8
   jmp   prtsp

dotransfer:
.ifp02
   sta   (ADDR0),y
.else
   sta   (ADDR0)
.endif
   rts

getrange:
   ; get addressrange <from> <to> into ADDR1, ADDR2
   ; and also setup function to call in loop
   sta   jmpcmd+1
   sty   jmpcmd+2
   lda   #$4c
   sta   jmpcmd+0
   jsr   getaddr
   bcs   error
   sta   ADDR1+0
   sty   ADDR1+1
   jsr   getaddr
   bcs   error
   sta   ADDR2+0
   sty   ADDR2+1
   rts

.assert >dofill     = >docompare, warning, "dofill and docompare are not on same page"
.assert >dohunt     = >docompare, warning, "dohunt and docompare are not on same page"
.assert >dotransfer = >docompare, warning, "dotranfer and docompare are not on same page"
