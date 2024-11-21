
.include "../native.inc"
.include "../native_bios.inc"
.include "../native_kernel.inc"

.segment "CODE"

escbuffer = $0200       ; longest sequence: $1b,"[000;000m",0
                        ;                       01234567890 = 11 bytes
                        ; shared with free block bitmap during cpmsave
convtmp = escbuffer
ESC := $1b

.global  vt100_tests

vt100:
   cpy   #VT100_CPOS_SOL
   bne   :+
   ; convenience function: set cursor to start of line
   ldy   #VT100_CPOS_GET   ; step 1: write cursor position to return values
   jsr   vt100             ; A=row
   ldx   #$01              ; column count starts at 1
   ldy   #VT100_CPOS_SET   ; step 2: set cursor to left position in same row
   ; slip through
:
   cpy   #VT100_SCRN_SIZ
   bne   :+
   ; convenience function: get size of window
   ldy   #VT100_CPOS_SAV   ; step 1: save cursor position
   jsr   vt100
   lda   #$fe
   tax
   ldy   #VT100_CPOS_SET   ; step 2: set cursor to bottom right position
   jsr   vt100
   ldy   #VT100_CPOS_GET   ; step 3: write cursor position to return values
   jsr   vt100
   ldy   #VT100_CPOS_RST   ; step 4: restore cursor position
:
   pha
   lda   #$44           ; save page $0200
   sta   XRAMSW
   pla
   cpy   #VT100_CPOS_GET
   bcs   @simple
   bcc   vt100_2param

@simple:
   lda   escfirst-3,y   ; Y contains offset, see table below
   sta   escbuffer+2
   lda   escsecond-3,y  ; second character, if required
   sta   escbuffer+3
.ifp02
   lda   #$00
   sta   escbuffer+4    ; make sure, string is NULL terminated
.else
   stz   escbuffer+4    ; make sure, string is NULL terminated
.endif

printbuffer:
   lda   #ESC           ; create escape sequence, start with ESC
   sta   escbuffer+0
   lda   #'['           ; followed by "["
   sta   escbuffer+1
   ldx   #$00
:
   lda   escbuffer,x
   beq   @end
   jsr   CHROUT
   inx
   bne   :-
@end:
   cpy   #$03
   beq   vt100_getreply
   bne   vt100_done

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
.ifp02
   sty   @restorey+1
   stx   @restorex+1
.else
   phy                  ; save command index on stack
   phx                  ; save second parameter on stack
.endif

   ldx   #$02
   jsr   bin2ascii

   lda   #';'
   sta   escbuffer,x
   inx

.ifp02
@restorex:
   lda   #$00
   jsr   bin2ascii

@restorey:
   ldy   #$00
.else
   pla                  ; get second parameter from stack
   jsr   bin2ascii

   ply                  ; get command index from stack
.endif
   lda   esclast,y
   sta   escbuffer,x
.ifp02
   lda   #$00
   sta   escbuffer+1,x
   beq   printbuffer
.else
   stz   escbuffer+1,x
   bra   printbuffer
.endif

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
.ifp02
   stx   @restorex+1
   dex
   jsr   ascii2bin
@restorex:
   ldx   #$00
.else
   phx
   dex
   jsr   ascii2bin
   plx
.endif
   sta   BRK_SA
:
   inx
   lda   escbuffer,x
   jsr   isdigit
   bcc   :-
   dex
   jsr   ascii2bin
   sta   BRK_SX
   ; slip through
vt100_done:
   lda   #$84           ; restore page $0200
   sta   XRAMSW
   ldx   BRK_SX         ; since we can be also called via JSR
   lda   BRK_SA         ; restore registers here as well
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

:
   sec
   rts
isdigit:
   cmp   #'0'
   bcc   :-
   cmp   #'9'+1
   rts
