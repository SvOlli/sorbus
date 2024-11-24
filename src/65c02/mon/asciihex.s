
.export     getaddr
.export     getbyte
.export     gethex8
.export     gethex4
.export     asc2hex

.import     skipspace
.importzp   ADDR0
.importzp   ADDR1
.importzp   ADDR2
.import     INBUF
.importzp   TMP8


.segment "CODE"

getaddr:
   jsr   skipspace
   lda   #$04           ; up to 4 digits
   sta   TMP8
   lda   #$00
   sta   ADDR0+0
   sta   ADDR0+1
@loop:
   lda   INBUF,x
   beq   @checkfail
   cmp   #' '
   beq   @exit
   jsr   asc2hex
   bcs   @checkfail
   inx
   asl
   asl
   asl
   asl
   ldy   #$04           ; 4 bits per digit
:
   asl
   rol   ADDR0+0
   rol   ADDR0+1
   dey
   bne   :-
   dec   TMP8
   bne   @loop
@exit:
   ; for convenience copy result to A/Y (lo/hi)
   ldy   ADDR0+1
   lda   ADDR0+0
   clc
   rts
@checkfail:
   lda   TMP8
   cmp   #$04
   bcc   @exit
@fail:
   rts

; read a byte from input buffer to A
getbyte:
   jsr   skipspace
   jsr   gethex4        ; get first hex digit
   bcs   @done          ; also C=1 on exit
   asl
   asl
   asl
   asl
   sta   TMP8
   lda   INBUF,x
   beq   @single
   cmp   #' '           ; check if it's a single digit number
   bne   @notsingle
@single:
   lda   TMP8
   lsr
   lsr
   lsr
   lsr
   bcc   @done          ; always true due to asls above, also C=0 on exit
@notsingle:
   jsr   gethex4
   bcs   @done          ; also C=1 on exit
   ora   TMP8
@done:
   rts

; replace with getbyte in papertape code?
gethex8:
   jsr   gethex4
   bcs   @error
   asl
   asl
   asl
   asl
   sta   TMP8
   jsr   gethex4
   bcs   @error
   ora   TMP8
@error:
   rts

gethex4:
   lda   INBUF,x
   inx
asc2hex:
   cmp   #'0'
   bcc   @fail
   cmp   #'9'+1
   bcc   :+
   and   #$df           ; uppercase
   cmp   #'A'
   bcc   @fail
   cmp   #'F'+1
   bcs   @fail
   ;clc
   adc   #$09           ; 'A' = $41 + 9 = $4A
:
   and   #$0f
   ;clc
   rts

@fail:
   sec
   lda   #$00
   rts
