
.include "jam.inc"
.include "jam_bios.inc"

MUSICINIT = $1000
MUSICPLAY = $1003

.segment "CODE"
   jsr   PRINT
   .byte 10,"This program plays music, with the SorbusSound Board"
   .byte 10,", running the SID emulation."
   .byte 10,"Timing is based on delay loops, so it's not that accurate"
   .byte 10,"Space will pause, ESC end"
   .byte 10,"---------------------------------------------------------", 0

   jsr @move

   lda #$00
   jsr MUSICINIT

:
   jsr   CHRIN
   cmp   #$1B           ; ESC
   beq   @done
   cmp   #$20           ; SPACE
   beq   :-

   jsr @pause
   inc @frame
   lda @frame
   cmp #50
   bne :+
   lda #$00
   sta @frame
   lda #'.'
   jsr CHROUT
:
   jsr MUSICPLAY
   jmp :--

@frame:
   .byte 0

@pause:

   ldx   #$10
   ldy   #$80
:
   iny                  ; 256 loop take ~1280 cycles
   bne   :-
   dex                  ; 16 loops should do it
   bne   :-   

   rts
   
@done:
   lda   #10            ; done, newline
   jsr   CHROUT

   jmp   ($FFFC)        ; do something useful: jump back to ROM

@move:
   ldx #$0b
   ldy #$00
@l1:   
   lda @music,y
@l2:   
   sta MUSICINIT,y
   dey 
   bne @l1
   inc @l1+2
   inc @l2+2
   dex
   bne @l1
   rts


; Music starts at $1000 , but has SID-Header in front
@music:
.incbin "Alter_Shice.sid",$7e
;.incbin "ulTOMATO.sid",$7e