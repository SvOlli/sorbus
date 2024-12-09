
.include "../native_bios.inc"
.include "../native.inc"
.include "../native_kernel.inc"

.export     hexenter
.export     hexupdown
.export     memorydump
.export     inbufhex4
.export     inbufhex8
.export     inbufaddr1
.export     inbufsp
.export     inbufa

.import     asc2hex
.import     getaddr
.import     getbyte
.import     getfirstnonspace
.import     prtsp
.import     prterr

.import     INBUF
.importzp   MODE
.importzp   ADDR0
.importzp   ADDR1
.importzp   ADDR2


.segment "CODE"

memorydump:
   jsr   getaddr
   bcs   memorydumpline
   ;and   #$f0           ; align output to 16 bytes
   sta   ADDR1+0
   sty   ADDR1+1
   jsr   getaddr
   bcs   memorydumpline

   ; calcuate number of bytes to be printed (2s complement)
   sec
   lda   ADDR1+0
   sbc   ADDR0+0
   sta   ADDR0+0
   lda   ADDR1+1
   sbc   ADDR0+1
   sta   ADDR0+1

   ; divide by 16 and we've got the number of lines (still 2s complement)
   ldy   #$04
:
   sec
   ror   ADDR0+1
   ror   ADDR0+0
   dey
   bne   :-

@addrloop:
   jsr   memorydumpline

   ; now check if end has been reached
   ; (2s complement for easier handling of wrapping)
   inc   ADDR0+0
   bne   @addrloop
   inc   ADDR0+1
   bne   @addrloop

@done:
   rts

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

   lda   ADDR1+0
   clc
   adc   #$10
   sta   ADDR1+0
   bcc   :+
   inc   ADDR1+1
:
   lda   #':'
   sta   MODE
   rts

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
   bcs   @lastprinted
   jsr   getaddr
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
   bne   :-             ; will always branch
@done:
   rts
@fail:
   jmp   prterr

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
   jsr   inbufaddr1

   ldy   #$00
:
   lda   #' '
   jsr   inbufsp
   lda   (ADDR1),y
   jsr   inbufhex8
   iny
   cpy   #$10
   bcc   :-
   lda   #$00
   jsr   inbufa
   rts

inbufaddr1:
   lda   ADDR1+1
   jsr   inbufhex8
   lda   ADDR1+0
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
