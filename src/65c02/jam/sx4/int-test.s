
.include "jam.inc"
.include "jam_bios.inc"

; A testsuite for different interrupt service routines

TMPVEC = $fe

SIN_AMPLITUDE = $fc
SIN_OFFSET = $fd

DECHI  = $fd

D_BUF  = $fb ; 3 bytes

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
   .byte 10,"d) $0c: line input (enter)"
   .byte 10,"e) $0c: line input (edit)"
   .byte 10,"f) $0d: generate sine"
   .byte 10,"g) $11: decimal print"
   .byte 10,"h) $0e: system monitor"
   .byte 10,"Ctrl+C) quit"
   .byte 10,0

menuloop:
   jsr   CHRIN
   cmp   #$03
   bne   :+
   jmp   ($FFFC)
:
   sec
   sbc   #'a'
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
   .word user
   .word dir
   .word vt100
   .word lineinput0
   .word lineinput
   .word gensine
   .word decimal
   .word sysmon
jmpend:

dec8:  ; print A as a decimal value
   ldx   #$00
dec16: ; print A/X (lo/hi) as a decimal value
   sta   TMPVEC+0
   stx   TMPVEC+1
   stz   D_BUF+0
   stz   D_BUF+1
   stz   D_BUF+2
   ldy   #$10
   sed
:
   asl   TMPVEC+0
   rol   TMPVEC+1
   ; lda + adc is the same as asl, only in decimal mode
   lda   D_BUF+0
   adc   D_BUF+0
   sta   D_BUF+0
   lda   D_BUF+1
   adc   D_BUF+1
   sta   D_BUF+1
   lda   D_BUF+2
   adc   D_BUF+2
   sta   D_BUF+2
   dey
   bne   :-
   cld

   ldx   #$02
:
   lda   D_BUF+0,x
   jsr   prhex8
   dex
   bpl   :-
   rts

prhex8: ; print a hex value, Y=0 -> skip leading zeros
   pha
   lsr
   lsr
   lsr
   lsr
   jsr   :+
   pla
:
   and   #$0f
   ora   #$30
   cmp   #'9'+1
   bcc   :+
   adc   #$06
:
   cmp   #$30
   bne   :+
   cpy   #$00
   bne   :+
   rts
:
   iny
   jmp   CHROUT

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

sysmon:
   jsr   PRINT
   .byte 10,"You can use ",$22,"G",$22," to return",0
   int   MONITOR
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
   jsr   PRINT
   .byte 10,"The old way:",0

   ldy   #VT100_CPOS_SAV
   int   VT100

   lda   #$fe
   tax
   ldy   #VT100_CPOS_SET
   int   VT100

   ldy   #VT100_CPOS_GET
   int   VT100
   ; position save to X and A here, and not changed until printing out

   ldy   #VT100_CPOS_RST
   int   VT100

   jsr   @printxa

   jsr   PRINT
   .byte "New convenience function:",0

   ldy   #VT100_SCRN_SIZ
   int   VT100

   jsr   @printxa

   jmp   done

@printxa:
   jsr   PRINT
   .byte 10,"terminal size ",0

   pha
   txa
   jsr   dec8

   lda   #'x'
   jsr   CHROUT

   pla
   jsr   dec8

   lda   #$0a
   jmp   CHROUT


lineinput0:
   stz   $CF00
lineinput:
   jsr   PRINT
   .byte 10,"        1234567890123456789012345678901234567890"
   .byte 10,"prompt> ",0

   lda   #$00
   ldx   #$CF
   ldy   #$28
   int   LINEINPUT

   php
   jsr   PRINT
   .byte 10,"A=",0
   int   PRHEX8

   tya
   jsr   PRINT
   .byte " Y=",0
   int   PRHEX8

   pla
   sta   D_BUF
   jsr   PRINT
   .byte " P(nv-bdizc)=",0
   ldy   #$08
:
   rol   D_BUF
   lda   #$18
   rol
   jsr   CHROUT
   dey
   bne   :-

   lda   #$0a
   jsr   CHROUT

   lda   #$cf
   jsr   hexdumppage

   jmp   done

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

gensine:
   ldy   #VT100_SCRN_CLR
   int   VT100
   lda   #$10
   sta   SIN_AMPLITUDE
   lda   #$00
   sta   SIN_OFFSET

@inputloop:
   lda   #$01
   tax
   ldy   #VT100_CPOS_SET
   int   VT100

   jsr   PRINT
   .byte "parameter:",0

   lda   SIN_AMPLITUDE
   ora   SIN_OFFSET
   int   PRHEX8

   lda   #$0a
   jsr   CHROUT

   lda   #$CF
   jsr   hexdumppage

   jsr   PRINT
   .byte 10,"0-9,a-f) amplitude   i-l) offset   q) quit"
   .byte 10,0

@keyloop:
   jsr   CHRIN
   bcs   @keyloop

   cmp   #'q'
   bne   :+
   jmp   start
:
   cmp   #'0'
   bcc   @keyloop
   cmp   #'9'+1
   bcc   @amp09

   cmp   #'a'
   bcc   @keyloop
   cmp   #'f'+1
   bcc   @ampaf

   cmp   #'i'
   bcc   @keyloop
   cmp   #'l'+1
   bcc   @offset

   bra   @keyloop

@offset:
   and   #$07           ; $01-$04
   dec                  ; $00-$03
   ror
   ror
   ror                  ; $00,$40,$80,$c0
   pha
   sta   SIN_OFFSET
   bra   @calcsine

@ampaf:
   ;clc          ; called via bcc
   adc   #$09    ; 'a'=$61, +9=$6a
@amp09:
   and   #$0f
   bne   :+
   lda   #$10    ; 0 is interpreted as $10
:
   sta   SIN_AMPLITUDE

@calcsine:
; A=page for table
; X=bits 0-4: size ($01-$10)
;   bit    5: write decimal parts to following page
;   bits 6-7: variant (offset in 90 degrees)
   lda   SIN_AMPLITUDE
   ora   SIN_OFFSET
   tax
   lda   #$CF
   int   GENSINE

   jmp   @inputloop

decimal:
   stz   DECHI
   lda   #$00
   sec
:
   ldx   DECHI
   php
   jsr   :+
   plp
   rol
   rol   DECHI
   bcc   :-

   dec
   tax
   jsr   :+
   jmp   done

:
   int   PRHEX16
   pha
   lda   #$20
   jsr   CHROUT
   pla
   int   PRDEC16
   pha
   lda   #$0a
   jsr   CHROUT
   pla
   rts
