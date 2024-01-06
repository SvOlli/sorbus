
.segment "CODE"

.include "native.inc"
.include "native_bios.inc"
.include "native_kernel.inc"

escbuffer = $0200       ; longest sequence: $1b,"[000;000m",0
                        ;                       01234567890 = 11 bytes
                        ; shared with free block bitmap during cpmsave
convtmp = escbuffer
ESC := $1b

.global  vt100_tests

vt100:
   cpy   #$03
   bcs   @simple
   jmp   vt100_2param

@simple:
   lda   escfirst-3,y    ; Y contains offset, see table below
   sta   escbuffer+2
   lda   escsecond-3,y   ; second character, if required
   sta   escbuffer+3
   stz   escbuffer+4     ; make sure, string is NULL terminated

printbuffer:
   lda   #ESC            ; create escape sequence, start with ESC
   sta   escbuffer+0
   lda   #'['            ; followed by "["
   sta   escbuffer+1
   ldx   #$00
:
   lda   escbuffer,x
   beq   @end
   jsr   CHROUT
   inx
   bra   :-
@end:
   cpy   #$03
   beq   vt100_getreply
   rts

    ; from left to right
    ; 3) get cursor
    ; 4) clear screen
    ; 5) clear to eol
    ; 6) reset scroll area
    ; 7) scroll down
    ; 8) scroll up
    ; 9) save
    ;10) unsave
escfirst:
   .byte "620rTSsu"
escsecond:
   .byte "nJK",0,0,0,0,0
esclast:
   .byte "Hrm" ; 0) cursor, 1) scroll, 2) colors

vt100_2param:
   phy                  ; save command index on stack
   phx                  ; save second parameter on stack

   ldx   #$02
   jsr   bin2ascii

   lda   #';'
   sta   escbuffer,x
   inx

   pla                  ; get second parameter from stack
   jsr   bin2ascii

   ply                  ; get command index from stack
   lda   esclast,y
   sta   escbuffer,x

   stz   escbuffer+1,x
   bra   printbuffer

vt100_getreply:
   ldx   #$00
   ldy   #$01
:
   jsr   CHRIN
   bcs   :-
   sta   escbuffer,x
   inx
   cmp   #'R'           ; change to ZP address when parameter required
   bne   :-

   ldx   #$01
:
   inx
   lda   escbuffer,x
   jsr   isdigit
   bcc   :-
   phx
   dex
   jsr   ascii2bin
   sta   $ffe
   sta   BRK_SA
   plx
:
   inx
   stx   $ff8
   lda   escbuffer,x
   jsr   isdigit
   bcc   :-
   dex
   jsr   ascii2bin
   sta   BRK_SX
   sta   $fff
   rts

bin2ascii:
   ; enter with A = value, X = position of hundreds
   sta   convtmp
   lda   #$00
   sta   escbuffer+1,x
   sta   escbuffer+2,x
   ldy   #$08
   sed
:
   asl   convtmp        ; binary value
   lda   escbuffer+2,x
   adc   escbuffer+2,x
   sta   escbuffer+2,x
   lda   escbuffer+1,x
   adc   escbuffer+1,x
   sta   escbuffer+1,x
   dey
   bne   :-
   cld
   ; hundreds
   lda   escbuffer+1,x
   ora   #'0'
   sta   escbuffer+0,x

   ; tens
   lda   escbuffer+2,x
   lsr
   lsr
   lsr
   lsr
   ora   #'0'
   sta   escbuffer+1,x

   ; ones
   lda   escbuffer+2,x
   and   #$0f
   ora   #'0'
   sta   escbuffer+2,x
   inx
   inx
   inx
   ; leave with X pointing to next buffer element
   rts

ascii2bin:
   ; enter with X = position of ones

   ; start with ones
   lda   escbuffer-0,x
   and   #$0f
   sta   convtmp

   ; now go for tens
   lda   escbuffer-1,x
   jsr   isdigit
   bcs   a2b_done

   and   #$0f
   tay
   lda   a2b_10,y
   adc   convtmp
   sta   convtmp
    
   ; finally got for 100s
   lda   escbuffer-2,x
   jsr   isdigit
   bcs   a2b_done

   and   #$0f
   cmp   #$03
   bcs   :+             ; >= 300 will never fit -> set carry
   tay
   lda   a2b_100,y
   adc   convtmp        ; result > 255 won't fit in 8 bit -> overflow set carry
   sta   convtmp
:
   ; so, when carry is set, result is invalid
   rts

a2b_done:
   lda   convtmp
   ; no hundrets, no overflow
   clc
   rts
a2b_100:
   .byte 0,100,200
a2b_10:
   .byte 0,10,20,30,40,50,60,70,80,90

isdigit:
   cmp   #'9'+1
   bcs   :+
   cmp   #'0'
   rol
   eor   #$01 ; negate carry, keep A
   ror
:
   ; carry set indicates not a digit
   rts
