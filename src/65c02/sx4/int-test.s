
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
   ; $01: CHRINUC
   ; $02: CHRCFG
   ; $03: PRHEX8
   ; $04: PRHEX16
   ; $05: CPMNAME
   ; $06: CPMLOAD
   ; $07: CPMSAVE
   ; $08: CPMERASE
   .byte 10,"b) $09: directory"
   .byte 10,"c) $0a: VT100"
   ; $0b: COPYBIOS
   .byte 10,"d) $0c: line input"
   ; $0d: GENSINE
   .byte 10,"`) quit"
   .byte 10,0

menuloop:
   jsr   CHRIN
   sec
   sbc   #'`'
   cmp   #<(jmpend-jmptab)/2
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
   .word vt100
   .word lineinput
jmpend:

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

vt100:
   ldy   #VT100_CPOS_SAV
   int   VT100

   lda   #$fe
   tax
   ldy   #VT100_CPOS_SET
   int   VT100

   ldy   #VT100_CPOS_GET
   int   VT100
   sta   TMPVEC+0
   stx   TMPVEC+1

   ldy   #VT100_CPOS_RST
   int   VT100

   jsr   PRINT
   .byte 10,"terminal size $",0

   lda   TMPVEC+1
   int   PRHEX8

   jsr   PRINT
   .byte " x $",0

   lda   TMPVEC+0
   int   PRHEX8

   lda   #$0a
   jsr   CHROUT

   jmp   done

lineinput:
   jsr   PRINT
   .byte 10,"        1234567890123456789012345678901234567890"
   .byte 10,"prompt> ",0

   lda   #$00
   ldx   #$CF
   ldy   #$28
   int   LINEINPUT

   bcc   :+
   jsr   PRINT
   .byte 10,"Ctrl-C ",0
:
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
