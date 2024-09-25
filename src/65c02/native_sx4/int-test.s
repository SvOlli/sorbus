
.include "../native_bios.inc"

TMPVEC = $fe

.segment "CODE"
start:
   ldy   #VT100_SCRN_CLR
   int   VT100

   lda   #$01
   tax
   ldy   #VT100_CPOS_SET
   int   VT100

   jsr   PRINT
   .byte 10,"Select test:"
   .byte 10,"a) $09: directory"
   .byte 10,"b) $0b: line input"
   .byte 10,"`) quit"
   .byte 10,0

menuloop:
   jsr   CHRIN
   sec
   sbc   #'`'
   cmp   #$03
   bcs   menuloop
   asl
   tax
   jmp   (jmptab,x)

done:
   jsr   PRINT
   .byte 10,"press any key ",0
:
   jsr   CHRIN
   bcs   :-
   jmp   start


jmptab:
   .word quit
   .word dir
   .word lineinput

dir:
   jsr   PRINT
   .byte "displaying directory on screen",10,0
   stz   CPM_SADDR+1
   ldy   #$0a
   int   CPMDIR

   jsr   PRINT
   .byte "loading directory to $9000",10,0
   stz   CPM_SADDR+0
   lda   #$90
   sta   CPM_SADDR+1
   ldy   #$0a
   int   CPMDIR

   lda   #$90
   jsr   hexdumppage

   jmp   done

lineinput:
   jsr   PRINT
   .byte 10,"        1234567890123456789012345678901234567890"
   .byte 10,"prompt> ",0

   lda   #$00
   ldx   #$CF
   ldy   #$28
   int   LINEINPUT

   jmp   done

quit:
   jmp   ($FFFC)

hexdumppage:
   stz   TMPVEC+0
   sta   TMPVEC+1

   ldy   #$00
@addrloop:
   lda   #$0a
   jsr   CHROUT

   tya
   clc
   adc   TMPVEC+0

   ldx   TMPVEC+1
   bcc   :+
   inx
:
   int   PRHEX16

   lda   #':'
   jsr   CHROUT

@dataloop:
   lda   #' '
   jsr   CHROUT

   lda   (TMPVEC),y
   int   PRHEX8

   iny
   beq   @done
   tya
   and   #$0f
   beq   @addrloop
   cmp   #$08
   bne   @nospc
   lda   #' '
   jsr   CHROUT
@nospc:
   bra   @dataloop

@done:
   lda   #$0a
   jsr   CHROUT

   rts
