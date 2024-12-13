
.include "jam_bios.inc"

.export     papertape

.import     gethex8
.import     prterr

.importzp   MODE
.importzp   ADDR0
.importzp   ADDR1

.import     INBUF


.segment "CODE"

addcksum:
   ; simple 8-bit to 16-bit addition
   ;clc                 ; C=0 from jsr gethex8
   adc   ADDR0+0
   sta   ADDR0+0
   bcc   :+
   inc   ADDR0+1
:
   rts

papertape:
   ; check if we're in fail mode
   jsr   gethex8
   bcs   @error
   beq   @okay          ; end marker ";00" (no data) clears error flag

   bit   MODE           ; stop papertape access after error
   bpl   :+             ; until any successful command

   ldx   #$02
   bne   @error
:

   sta   MODE           ; use MODE for counter, will be cleared at end
   sta   ADDR0+0        ; init checksum
   ldy   #$00           ; since Y=$00 is required
   sty   ADDR0+1        ; use for clean out hi-byte of checksum as well

   jsr   gethex8
   bcs   @error
   sta   ADDR1+1        ; got hi-byte off address
   jsr   addcksum
   jsr   gethex8
   bcs   @error
   sta   ADDR1+0        ; got lo-byte off address
   jsr   addcksum

@dataloop:
   jsr   gethex8
   bcs   @error
   sta   (ADDR1),y      ; store data at address
   jsr   addcksum
   iny
   cpy   MODE           ; got all bytes?
   bcc   @dataloop

   jsr   gethex8
   bcs   @error
   cmp   ADDR0+1        ; verify checksum hi-byte
   bne   @error

   jsr   gethex8
   bcs   @error
   cmp   ADDR0+0        ; verify checksum lo-byte
   bne   @error

@okay:
   lda   #';'
   sta   MODE
   rts

@error:
   lda   #';' + $80     ; $80 indicates error
   sta   MODE
   ; typically a false error keeps INBUF to correct typo -> not here!
.ifp02
   lda   #$00
   sta   INBUF
.else
   stz   INBUF
.endif
   dex
   jmp   prterr
