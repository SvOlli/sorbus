
.include "fb32x32.inc"

.define DELAY 500
.define DEBUGOUTPUT 0
.define DIVLOOPS 5

FRAMEBUFFER    := $cc00
SINE           := $c000
CACHE          := $c100
CACHE_H        := CACHE
CACHE_V        := CACHE+$20

localram       := $10

; I never though that I require 64 bytes just for variables for a
; plasma effect. Totally worth it!

hafstart        = localram+$00
hbfstart        = localram+$02
hacurr          = localram+$04
hbcurr          = localram+$06
vafstart        = localram+$08
vbfstart        = localram+$0a
vacurr          = localram+$0c
vbcurr          = localram+$0e

tabadd          = localram+$10
hafadd          = tabadd+$0
hbfadd          = tabadd+$2
haadd           = tabadd+$4
hbadd           = tabadd+$6
vafadd          = tabadd+$8
vbfadd          = tabadd+$a
vaadd           = tabadd+$c
vbadd           = tabadd+$e

tabnext         = localram+$20
hafnext         = tabnext+$0
hbfnext         = tabnext+$2
hanext          = tabnext+$4
hbnext          = tabnext+$6
vafnext         = tabnext+$8
vbfnext         = tabnext+$a
vanext          = tabnext+$c
vbnext          = tabnext+$e

tabstep         = localram+$30
hafstep         = tabstep+$0
hbfstep         = tabstep+$2
hastep          = tabstep+$4
hbstep          = tabstep+$6
vafstep         = tabstep+$8
vbfstep         = tabstep+$a
vastep          = tabstep+$c
vbstep          = tabstep+$e

paramend        = localram+$40

frmcnt          = paramend+$00
steps_left      = paramend+$01
vector          = paramend+$02
tmp8            = paramend+$04

.segment "DATA"


plasma_init_vals:
;               horizontal              vertical
;        fadd, fadd,  add,  add, fadd, fadd,  add,  add
   .word $0000,$0000,$0800,$0800,$0000,$0000,$0800,$0800

.segment "CODE"

start:
   lda   #$07
   sta   FB32X32_COLMAP
   lda   #$e0
   sta   frmcnt

   jsr   PRINT
   .byte 10,"(CTRL-C to quit) ",10,0

   jsr   plasma_init
   
   sei
   lda   #<irqhandler
   sta   UVNBI+0
   lda   #>irqhandler
   sta   UVNBI+1
   lda   #<DELAY
   sta   TMIMRL
   lda   #>DELAY
   sta   TMIMRH
   cli

@mainloop:
   jsr   CHRIN
   bcs   @mainloop

   cmp   #'0'
   bcc   @nodigit
   cmp   #'9'+1
   bcs   @nodigit
   and   #$0f
   sta   FB32X32_COLMAP
   bra   @mainloop

@nodigit:
   cmp   #$03
   bne   @mainloop

   stz   TMIMRL
   stz   TMIMRH

   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$01
   int   FB32X32

   jmp   ($fffc)


irqhandler:
   pha
   phx
   phy

   jsr   plasma_calc
   inc   frmcnt
   bne   :+
   jsr   plasma_next
:
   lda   TMIMRL         ; acknoledge timer

   ply
   plx
   pla
   rti


plasma_next:
   jsr   PRINT
   .byte "next:",0

   ldx   #$00
   bra   :++
:
   lda   #','
   jsr   CHROUT
:
   lda   #'$'
   jsr   CHROUT
   lda   RANDOM
   sta   hafnext+0,x
   lda   RANDOM
   cmp   #$80
   ror
   ror   hafnext+0,x
   cmp   #$80
   ror
   ror   hafnext+0,x
   cmp   #$80
   ror
   ror   hafnext+0,x
   sta   hafnext+1,x
   phx
   lda   hafnext+1,x
   sta   tmp8
   lda   hafnext+0,x
   ldx   tmp8
   int   PRHEX16
   plx
   inx
   inx
   cpx   #$10
   bcc   :--

   lda   #$0a
   jsr   CHROUT
   
   ldx   #$00

@calcsteps:
   sec
   lda   tabnext+0,x       ; 16 bit addition
   sbc   tabadd+0,x
   sta   tabstep+0,x
   lda   tabnext+1,x
   sbc   tabadd+1,x
   ldy   #DIVLOOPS         ; divide
:
   cmp   #$80
   ror
   ror   tabstep+0,x
   dey
   bne   :-
   sta   tabstep+1,x
   inx
   inx
   cpx   #$10
   bcc   @calcsteps

.if DEBUGOUTPUT
   ldy   #$00
:   
   lda   tabstep+1,y
   tax
   lda   tabstep+0,y
   int   PRHEX16
   lda   #$20
   jsr   CHROUT
   iny
   iny
   cpy   #$10
   bcc   :-
   lda   #'S'
   jsr   CHROUT
   lda   #$0a
   jsr   CHROUT
.endif

   lda   #(1 << DIVLOOPS)
   sta   steps_left
   
   rts

plasma_init:
   lda   #$07
   sta   FB32X32_COLMAP
   stz   steps_left

   lda   #$90
   ldx   #>SINE
   int   GENSINE

   ldx   #$0f
:
   stz   hafstart,x
   lda   plasma_init_vals,x
   sta   hafadd,x
   sta   hafnext,x
   dex
   bpl   :-

   jsr   PRINT
   .byte "strt:",0

   ldx   #$00
   bra   :++
:
   lda   #','
   jsr   CHROUT
:
   lda   #'$'
   jsr   CHROUT
   phx
   lda   hafnext+1,x
   sta   tmp8
   lda   hafnext+0,x
   ldx   tmp8
   int   PRHEX16
   plx
   inx
   inx
   cpx   #$10
   bcc   :--

   lda   #$0a
   jsr   CHROUT

   ; fall through

plasma_calc_axis:
   stz   vector+0
   lda   #>CACHE
   sta   vector+1

   ldx   #$00
   jsr   @subcalc

   ldx   #<(hafstart-vafstart)
@subcalc:
   stx   @offset+1
   clc
   lda   vafstart+0,x
   adc   vafadd  +0,x
   sta   vafstart+0,x
   sta   vacurr  +0,x
   lda   vafstart+1,x
   adc   vafadd  +1,x
   sta   vafstart+1,x
   sta   vacurr  +1,x
   clc
   lda   vbfstart+0,x
   adc   vbfadd  +0,x
   sta   vbfstart+0,x
   sta   vbcurr  +0,x
   lda   vbfstart+1,x
   adc   vbfadd  +1,x
   sta   vbfstart+1,x
   sta   vbcurr  +1,x

   lda   #$20
   sta   tmp8
@loop:
   clc
@offset:
   ldx   #$00

   clc
   lda   vacurr+0,x
   adc   vaadd +0,x
   sta   vacurr+0,x

   lda   vacurr+1,x
   adc   vaadd +1,x
   sta   vacurr+1,x
   tay

   clc
   lda   vbcurr+0,x
   adc   vbadd +0,x
   sta   vbcurr+0,x

   lda   vbcurr+1,x
   adc   vbadd +1,x
   sta   vbcurr+1,x
   tax

   clc
   lda   SINE,y
   adc   SINE,x
   ror
   sta   (vector)
   inc   vector+0

   dec   tmp8
   bne   @loop
   rts


plasma_calc:
   jsr   plasma_calc_axis  ; cannot be inlined due to jsr inside routine

   stz   vector+0
   lda   #>FRAMEBUFFER
   sta   vector+1

   ldy   #$1f
@loopy:
   ldx   #$1f
@loopx:
   clc
   lda   CACHE_H,x
   adc   CACHE_V,y
   ror
   sta   (vector)
   inc   vector+0
   bne   :+
   inc   vector+1
:
   dex
   bpl   @loopx
   dey
   bpl   @loopy

   lda   steps_left
   bne   @adjust
   ldx   #$0f
:
   lda   hafnext,x
   sta   hafadd,x
   dex
   bpl   :-
   bra   @done

@adjust:
   dec   steps_left
   ldx   #$00
:
   clc
   lda   tabadd +0,x
   adc   tabstep+0,x
   sta   tabadd +0,x
   lda   tabadd +1,x
   adc   tabstep+1,x
   sta   tabadd +1,x
   inx
   inx
   cpx   #$10
   bcc   :-

.if DEBUGOUTPUT
   ldy   #$00
:   
   lda   tabadd+1,y
   tax
   lda   tabadd+0,y
   int   PRHEX16
   lda   #$20
   jsr   CHROUT
   iny
   iny
   cpy   #$10
   bcc   :-
   lda   #$0a
   jsr   CHROUT
.endif

@done:
   stz   FB32X32_SRC+0
   lda   #>FRAMEBUFFER
   sta   FB32X32_SRC+1

   stz   FB32X32_DEST_X
   stz   FB32X32_DEST_Y

   lda   #$1f
   sta   FB32X32_STEP
   sta   FB32X32_WIDTH
   sta   FB32X32_HEIGHT

   stz   FB32X32_COPY
   rts
