
.include "native.inc"
.include "native_bios.inc"
.include "native_cpmfs.inc"
.include "native_kernel.inc"

.segment "CODE"

readp    = $10
endp     = $12
index    = $14
lastidx  = $15
lastp    = $16

FIRSTLINE   := $03      ; start at 3rd line on screen with filenames
USER        := $0a      ; SX4 files are on user 10
DIRSTART    := $0400    ; must be on page boundry

loader:
   ldx   #$00
:
   stz   readp,x
   inx
   bpl   :-

   stz   cpm_saddr+0
   stz   readp+0
   stz   endp+0
   lda   #>DIRSTART
   sta   cpm_saddr+1
   sta   readp+1
   sta   endp+1

   ldy   #USER
   int   CPMDIR

@loop:
   ldy   #$0c
   ldx   #$04
:
   lda   @filesig-1,x
   cmp   (readp),y
   bne   @next
   dey
   dex
   bne   :-

   ldy   #$0f
:
   lda   (readp),y
   sta   (endp),y
   dey
   bpl   :-

   clc
   lda   endp+0
   adc   #$10
   sta   endp+0
   bcc   :+
   inc   endp+1
:

@next:
   clc
   lda   readp+0
   adc   #$10
   sta   readp+0
   bcc   :+
   inc   readp+1
:
   lda   readp+0
   cmp   cpm_eaddr+0
   bne   @loop
   lda   readp+1
   cmp   cpm_eaddr+1
   bne   @loop

   lda   #$00
   sec
   sbc   endp+0

   tay
:
   beq   @clrdone
   lda   #$e5
   dey
   sta   (endp),y
   bne   :-
@clrdone:

   lda   endp+1
   cmp   #>DIRSTART
   bne   :+
   lda   endp+0
   bne   :+
   jmp   @nofiles

:
   ldx   endp+1
   lda   endp+0
   beq   :+
   inx
:
   stx   lastp

   ldy   #VT100_SCRN_CLR
   int   VT100

   lda   #$01
   jsr   @setline

   jsr   PRINT
   .byte "Sorbus Program Loader",0

   lda   #>DIRSTART
   stz   readp+0
   sta   readp+1

@prpage:
   lda   #$10
   sta   lastidx

@prent:
   lda   readp+0
   lsr
   lsr
   lsr
   lsr
   adc   #FIRSTLINE
   ldx   #$05
   ldy   #VT100_CPOS_SET
   int   VT100

   ldy   #$01
@prname:
   lda   (readp),y
   beq   @nextent
   cmp   #$e5
   beq   @noent
   jsr   CHROUT
   cpy   #$08
   bne   :+
   lda   #'.'
   jsr   CHROUT
:
   iny
   cpy   #$0c
   bcc   @prname
   bra   @nextent

@noent:
   dec   lastidx
   lda   #' '
:
   jsr   CHROUT
   iny
   cpy   #$0d
   bcc   :-

@nextent:
   lda   readp+0
   clc
   adc   #$10
   sta   readp+0
   bne   @prent

@setarrow:
   lda   index
   clc
   adc   #FIRSTLINE-1
   jsr   @setline
   jsr   PRINT
   .byte "   ",10," =>",10,"   ",0

   lda   #FIRSTLINE+$10
   jsr   @setline

@inputloop:
   int   CHRINUC
   cmp   #$1b           ; ESC
   beq   @esc
   cmp   #$03           ; CTRL+C
   beq   @leave
   cmp   #$0d           ; RETURN
   bne   @inputloop

@select:
   lda   index
   asl
   asl
   asl
   asl
   adc   #$01
   ldx   readp+1

   ldy   #USER
   int   CPMNAME
   int   CPMLOAD

   jmp   $0400

@esc:
   int   CHRINUC
   cmp   #$1b
   beq   @leave
   cmp   #'['
   bne   @inputloop+2

   int   CHRINUC
   cmp   #'A'
   beq   @up
   cmp   #'B'
   beq   @down
   cmp   #'C'
   beq   @right
   cmp   #'D'
   beq   @left
   bne   @inputloop+2

@left:
   ldx   readp+1
   dex
   cpx   #>DIRSTART
   bcc   :+
   stx   readp+1
:
   bra   @prpage2
@right:
   ldx   readp+1
   inx
   cpx   lastp
   bcs   @prpage2
   stx   readp+1
@prpage2:
   jmp   @prpage
@down:
   ldx   index
   inx
   cpx   lastidx
   bcs   :+
   stx   index
:
   bra   @setarrow2
@up:
   ldx   index
   dex
   bmi   @setarrow2
   stx   index
@setarrow2:
   jmp   @setarrow

@setline:
   ldx   #$01
   ldy   #VT100_CPOS_SET
   int   VT100
   rts

@nofiles:
   jsr   PRINT
   .byte 10, "no files found",10, 0
@leave:
   jmp   $E000

@filesig:
   .byte "SX4",0
