
.export     papertape

.import     CHROUT
.import     gethex8

.importzp   MODE
.importzp   ADDR1
.importzp   ADDR2
.importzp   ASAVE


.segment "CODE"

addcksum:
   adc   ADDR2+0
   sta   ADDR2+0
   bcc   :+
   inc   ADDR2+1
:
   rts

papertape:
   ; check if we're in fail mode
   jsr   gethex8
   bcs   error
   bne   :+
   lda   #$00           ; ???
   sta   MODE
   rts

:
   bit   MODE
   bmi   error
   sta   ASAVE          ; counter
   sta   ADDR2+0        ; checksum
   ldy   #$00
   sty   ADDR2+1

   jsr   gethex8
   bcs   error
   sta   ADDR1+1  ;
   jsr   addcksum
   jsr   gethex8
   bcs   error
   sta   ADDR1+0  ;
   jsr   addcksum

@dataloop:
   jsr   gethex8
   bcs   error
   sta   (ADDR1),y
   jsr   addcksum
   iny
   cpy   ASAVE
   bcc   @dataloop

   jsr   gethex8
   bcs   error
   cmp   ADDR2+1
   bne   error

   jsr   gethex8
   bcs   error
   cmp   ADDR2+0
   bne   error

   lda   ADDR1+1
   sta   ADDR2+1
   lda   ADDR1+0
   sta   ADDR2+0
   rts

error:
   lda   #'?'
   jsr   CHROUT

   lda   MODE
   ora   #$80
   sta   MODE
   rts
