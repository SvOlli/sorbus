
.include "../native.inc"
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
   .byte 10,"a) $00: user"
   .byte 10,"b) $09: directory"
   .byte 10,"c) $0b: line input"
   .byte 10,"`) quit"
   .byte 10,0

menuloop:
   jsr   CHRIN
   sec
   sbc   #'`'
   cmp   #$04
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
   .word user
   .word dir
   .word lineinput

user:
   jsr   PRINT
   .byte "save vector:$",$00
   lda   UVBRK+0
   ldx   UVBRK+1
   pha
   phx
   int   PRHEX16

   lda   #$0a
   jsr   CHROUT

   jsr   PRINT
   .byte "set vector:$",$00
   lda   #<userbrk
   ldx   #>userbrk
   sta   UVBRK+0
   stx   UVBRK+1
   int   PRHEX16

   lda   #$0a
   jsr   CHROUT

   int   INTUSER
   int   $80

   jsr   PRINT
   .byte "restore vector:$",$00
   plx
   pla
   sta   UVBRK+0
   stx   UVBRK+1
   int   PRHEX16

   lda   #$0a
   jsr   CHROUT
   jmp   done

userbrk:
   lda   ASAVE
   pha
   jsr   PRINT
   .byte "user brk routine: ",$00
   pla
   int   PRHEX8
   lda   #$0a
   jmp   CHROUT

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

@addrloop:
   ldy   #$00
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
   cpy   #$08
   bne   :+
   jsr   CHROUT
:
   jsr   CHROUT

   lda   (TMPVEC),y
   int   PRHEX8

   iny
   cpy   #$10
   bcc   @dataloop

@ascii:
   lda   #' '
   jsr   CHROUT
   jsr   CHROUT
   ldy   #$00
@asciiloop:
   lda   (TMPVEC),y
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

   lda   #$10
   clc
   adc   TMPVEC+0
   sta   TMPVEC+0
   bne   @addrloop

   lda   #$0a
   jmp   CHROUT
