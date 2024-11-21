
.export     go
.export     regdump
.export     regedit
.export     regsave
.export     regupdown
.export     inthandler

.import     CHROUT
.import     PRINT
.import     prthex8
.import     prtsp
.import     inbufhex8
.import     inbufsp
.import     inbufa
.import     getaddr
.import     getbyte
.import     newenter
.import     start

.importzp   R_PC
.importzp   R_A
.importzp   R_X
.importzp   R_Y
.importzp   R_SP
.importzp   R_P
.importzp   MODE
.import     INBUF


.segment "CODE"

regdump:
   jsr   PRINT
   .byte 10,"   PC  AC XR YR SP NV-BDIZC"
   ;           FFF2 14 43 45 FF 00000000
   .byte 10," ~",0
   lda   R_PC+1
   jsr   prthex8
   lda   R_PC+0
   jsr   prthex8
   
   ldx   #R_A
:
   jsr   prtsp
   lda   $00,x
   jsr   prthex8
   inx
   cpx   #R_P
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

inthandler:
.ifp02
   ; on NMOS 6502
   ; - called directly via IRQ vector @ $fffe/$ffff
   ; - adjust interrupt address for 1-byte BRK
   cld
   sta   R_A
   stx   R_X
   sty   R_Y
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
   stx   R_PC+0
   sty   R_PC+1
@nobrk:
   tsx
   stx   R_SP
   jmp   start
.else
   ; on CMOS 6502
   ; - called via BRK routine in kernel
   ; - BRK adjust is never required
   ; - stack offset is totally different
   ; now there are 7 extra bytes on the stack, when run with G
   ; - SP
   ; - return address lo from BIOS jumping into kernel (needs to be dropped)
   ; - return address hi from BIOS jumping into kernel (needs to be dropped)
   ; - P register from BRK
   ; - return address lo from BRK
   ; - return address hi from BRK
   ; - return address lo for RTS from "G"
   ; - return address hi for RTS from "G"
   cld
   sta   R_A
   stx   R_X
   sty   R_Y
   pla
   pla
   pla
   sta   R_P
   pla
   sta   R_PC+0
   pla
   sta   R_PC+1
   tsx
   stx   R_SP
   jmp   start
.endif

regupdown:
   ldx   #$00
   lda   #'~'
   jsr   inbufa
   lda   R_PC+1
   jsr   inbufhex8
   lda   R_PC+0
   jsr   inbufhex8

   ldy   #R_A
:
   jsr   inbufsp
   lda   $00,y
   jsr   inbufhex8
   iny
   cpy   #R_P
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
   bcs   @done
   sta   R_PC+0
   sty   R_PC+1

   ldy   #R_A
@regloop:
   jsr   getbyte        ; get A,X,Y,SP
   bcs   @done
   sta   $00,y
   iny
   cpy   #R_P
   bcc   @regloop

   ldy   #$08
@ploop:
   lda   INBUF,x
   beq   @done
   cmp   #'0'
   beq   :+
   cmp   #'1'
   bne   @done
:
   ror
   rol   MODE
   dey
   bne   @ploop

   ; successfully converted bits to byte
   lda   MODE
   sta   R_P
   
@done:
   jmp   exitsetmode

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
   ; set return address for RTS in code executed
   lda   #>(newenter-1)
   pha
   lda   #<(newenter-1)
   pha
   ; set start address to execute
   lda   R_PC+1
   pha
   lda   R_PC+0
   pha
   ; set processor status
   lda   R_P
   pha
   ; load registers
   lda   R_A
   ldx   R_X
   ldy   R_Y
   ; execute with set processor status
   rti
