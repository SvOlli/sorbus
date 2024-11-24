
.include "../native_bios.inc"
.include "../native.inc"
.include "../native_kernel.inc"

.export     go
.export     regdump
.export     regedit
.export     regsave
.export     regupdown

.import     prterr
.import     prthex8
.import     prtsp
.import     inbufhex8
.import     inbufsp
.import     inbufa
.import     getaddr
.import     getbyte
.import     clrenter
.import     newenter
.import     skipspace

.importzp   MODE
.import     INBUF

R_PC     = MON_PC
R_BK     = MON_BK
R_A      = MON_A
R_X      = MON_X
R_Y      = MON_Y
R_SP     = MON_SP
R_P      = MON_P

.ifp02
.define REGSTART $01    ; Start regdump at accumulator
.else
.define REGSTART $00    ; Start regdump at bank
.endif
.define NUMREGS <(R_SP-R_BK+1)

.segment "DATA"

txt_brk:
.ifp02
   .byte "BRK",0
.else
   .byte "BRK $",0
.endif
txt_init:
.ifp02
   .byte "boot",0
.else
   .byte "menu",0
.endif
txt_nmi:
   .byte "NMI",0
txt_irq:
   .byte "IRQ",0

.segment "CODE"

mon_nmi:
   sta   BRK_SA         ; need to save A by replicating IRQ behaviour
   lda   #<(txt_nmi-txt_brk)
   .byte $2c
mon_irq:
   lda   #<(txt_irq-txt_brk)
   .byte $2c
mon_brk:
   lda   #<(txt_brk-txt_brk)
   ; R_A is already saved, now save the rest
   sta   MODE           ; save index for source
   lda   BRK_SA
   sta   R_A
   stx   R_X
   sty   R_Y
.ifp02
   ; on NMOS, BRK is implemented as 1-byte -> adjust return value
   pla
   sta   R_P
   pla
   tax                 ; X:PCL
   pla
   tay                 ; Y:PCH
   lda   R_P
   and   #$10
   beq   @nobrk
   txa                 ; cpx #$00
   bne   :+            ; no hibyte decrement needed
   dey
:
   dex
@nobrk:
   stx   R_PC+0
   sty   R_PC+1
.else
   ; on CMOS, BRK is implemented via handler using JSR -> remove RTS
   pla
   pla
   pla
   sta   R_P
   ; on CMOS, BRK is implemented as 2-byte -> no adjust required
   pla
   sta   R_PC+0
   pla
   sta   R_PC+1
.endif
   tsx
   stx   R_SP
   ldy   MODE
   bpl   mon_hello      ; text offset should be <$80

   .byte $2c
mon_init:
   ldy   #<(txt_init-txt_brk)
   ldx   #$07
:
   lda   defregs,x
   sta   R_PC,x
   dex
   bpl   :-

mon_hello:
   sei                  ; sanitize
   cld                  ; sanitize
.ifp02
.else
   lda   (TMP16)
   phy
   pha
.endif
   jsr   PRINT
   .byte 10,"Sorbus System Monitor via ",0
:
   lda   txt_brk,y
   beq   :+
   jsr   CHROUT
   iny
   bne   :-
:
.ifp02
.else
   ; on CMOS get interrupt number
   pla
   ply
   bne   :+
   jsr   prthex8
:
.endif
   jsr   regdump
   jmp   clrenter


.segment "DATA"
defregs:
   .word $fff2          ; R_PC
   .byte $01            ; R_BK
   .byte $00            ; R_A
   .byte $00            ; R_X
   .byte $00            ; R_Y
   .byte $FF            ; R_SP
   .byte $26            ; R_P

.segment "CODE"

regdump:
   jsr   PRINT
.if REGSTART
   .byte 10,"   PC  AC XR YR SP NV-BDIZC"
   ;           FFF2 14 43 45 FF 00100110
.else
   .byte 10,"   PC  BK AC XR YR SP NV-BDIZC"
   ;           FFF2 00 14 43 45 FF 00100110
.endif
   .byte 10," ~",0
   lda   R_PC+1
   jsr   prthex8
   lda   R_PC+0
   jsr   prthex8

   ldx   #REGSTART
:
   jsr   prtsp
   lda   R_BK,x
   jsr   prthex8
   inx
   cpx   #NUMREGS
   bcc   :-

   jsr   prtsp
   lda   R_P
   sta   MODE
   ldx   #$08
:
   rol   MODE
   lda   #$18           ; '0' >> 1
   rol
   jsr   CHROUT
   dex
   bne   :-
exitsetmode:
   lda   #'~'
   sta   MODE
   rts

regsave:
   ; needs to be called via JSR from top level
   php
   sta   R_A
   stx   R_X
   sty   R_Y
   pla
   sta   R_P
   tsx
   inx                  ; adjust regsave is subroutine
   inx
   stx   R_SP
   rts

regupdown:
   ldx   #$00
   lda   #'~'
   jsr   inbufa
   lda   R_PC+1
   jsr   inbufhex8
   lda   R_PC+0
   jsr   inbufhex8

   ldy   #REGSTART
:
   jsr   inbufsp
   lda   R_BK,y
   jsr   inbufhex8
   iny
   cpy   #NUMREGS
   bcc   :-

   jsr   inbufsp
   lda   R_P
   sta   MODE
   ldy   #$08
:
   rol   MODE
   lda   #$18
   rol
   jsr   inbufa
   dey
   bne   :-

   lda   #$00
   jsr   inbufa
   bne   exitsetmode    ; always true

regedit:
   jsr   getaddr        ; get PC
   bcs   @error
   sta   R_PC+0
   sty   R_PC+1

   ldy   #REGSTART
@regloop:
   jsr   getbyte        ; get A,X,Y,SP
   bcs   @error
   sta   R_BK,y
   iny
   cpy   #NUMREGS
   bcc   @regloop

   jsr   skipspace
   ldy   #$08           ; need to be at least 8 binary digits
@ploop:
   lda   INBUF,x
   beq   @error
   cmp   #'0'
   beq   :+
   cmp   #'1'
   bne   @error
:
   lsr                  ; shift the lowest bit of '0' or '1' to carry
   rol   MODE           ; rotate this one in
   inx                  ; next binary digit in buffer
   dey                  ; count digits to go
   bne   @ploop

   ; successfully converted bits to byte
   lda   MODE
   sta   R_P
@done:
   jmp   exitsetmode
@error:
   jsr   exitsetmode
   jmp   prterr

go:
   ; check if there is an address as parameter
   jsr   getaddr
   bcs   @noarg
   sta   R_PC+0
   sty   R_PC+1
@noarg:
   ; only command to return to start, so drop return address
   ldx   R_SP
   txs
   ; set start address to execute
   lda   R_PC+1
   pha
   lda   R_PC+0
   pha
   ldx   #$03
:
   lda   R_BK,x
   sta   BRK_SB,x
   dex
   bpl   :-

.ifp02
.else
   ; in ROM only: check if target bank is $00
   tax                  ; check if akku (last read R_BK) is $00
   bne   :+
   jsr   copybios       ; a jump to ram bank only works with BIOS in RAM
:
.endif

   ; execute with set processor status
   lda   R_P
   pha                  ; save an extra dummy for bankrti to drop
   pha
   plp
   jmp   bankrti
