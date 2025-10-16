
.include "fb32x32.inc"

.define DELAY 500

FRAMEBUFFER    := $cc00
SINE           := $c000
CACHE          := $c100
CACHE_H        := CACHE
CACHE_V        := CACHE+$20

localram       := $10

hafstart        = localram+$00
hbfstart        = localram+$02
hacurr          = localram+$04
hbcurr          = localram+$06
vafstart        = localram+$08
vbfstart        = localram+$0a
vacurr          = localram+$0c
vbcurr          = localram+$0e

hafadd          = localram+$10
hbfadd          = localram+$12
haadd           = localram+$14
hbadd           = localram+$16
vafadd          = localram+$18
vbfadd          = localram+$1a
vaadd           = localram+$1c
vbadd           = localram+$1e

vector          = localram+$20
tmp8            = localram+$22


.macro iface key,value,function
   .byte key
   .byte value
   .word function
.endmac

.segment "DATA"



plasma_init_vals:
;            horizontal              vertical
;         fadd, fadd,  add,  add, fadd, fadd,  add,  add
   .word $0600,$fa00,$fb00,$0200,$0700,$fd00,$0500,$fc00

interface:
   iface '1',hafadd+1,dec8
   iface '2',hafadd  ,dec16
   iface '3',hbfadd+1,dec8
   iface '4',hbfadd  ,dec16
   iface '5',haadd+1 ,dec8
   iface '6',haadd   ,dec16
   iface '7',hbadd+1 ,dec8
   iface '8',hbadd   ,dec16

   iface 'q',hafadd+1,inc8
   iface 'w',hafadd  ,inc16
   iface 'e',hbfadd+1,inc8
   iface 'r',hbfadd  ,inc16
   iface 't',haadd+1 ,inc8
   iface 'y',haadd   ,inc16
   iface 'u',hbadd+1 ,inc8
   iface 'i',hbadd   ,inc16

   iface 'a',vafadd+1,dec8
   iface 's',vafadd  ,dec16
   iface 'd',vbfadd+1,dec8
   iface 'f',vbfadd  ,dec16
   iface 'g',vaadd+1 ,dec8
   iface 'h',vaadd   ,dec16
   iface 'j',vbadd+1 ,dec8
   iface 'k',vbadd   ,dec16

   iface 'z',vafadd+1,inc8
   iface 'x',vafadd  ,inc16
   iface 'c',vbfadd+1,inc8
   iface 'v',vbfadd  ,inc16
   iface 'b',vaadd+1 ,inc8
   iface 'n',vaadd   ,inc16
   iface 'm',vbadd+1 ,inc8
   iface ',',vbadd   ,inc16

   iface '/',0       ,zero
   iface '.',0       ,rnd

interface_end:

.segment "CODE"

start:
   lda   #$07
   sta   FB32X32_COLMAP

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

   jsr   PRINT
   .byte 10,"(CTRL-C to quit) ",10,0

output:
   jsr   PRINT
   .byte ".word $",0

   ldy   #$00
   lda   #<hafadd
   sta   vector+0
   stz   vector+1
   bra   :++
:
   jsr   PRINT
   .byte ",$",0
:
   lda   (vector),y
   pha
   iny
   lda   (vector),y
   tax
   pla
   iny
   int   PRHEX16
   cpy   #$10
   bcc   :--

   lda   #$0a
   jsr   CHROUT

@mainloop:
   jsr   CHRIN
   bcs   @mainloop

   ldx   #$00
@keyloop:
   cmp   interface,x
   beq   @found
   inx
   inx
   inx
   inx
   cpx   #<(interface_end-interface)
   bcc   @keyloop

   cmp   #$03
   bne   @mainloop

   stz   TMIMRL
   stz   TMIMRH

   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$01
   int   FB32X32

   jmp   ($fffc)

@found:
   inx
   lda   interface,x
   sta   vector+0
   stz   vector+1
   inx
   jmp   (interface,x)

plasma_init:
   lda   #$90
   ldx   #>SINE
   int   GENSINE

   ldx   #$f
:
   lda   plasma_init_vals,x
   sta   hafadd,x
   dex
   bpl   :-

   rts

inc8:
   lda   (vector)
   inc
   sta   (vector)
   jmp   output

dec8:
   lda   (vector)
   dec
   sta   (vector)
   jmp   output

inc16:
   clc
   ldy   #$00
   lda   (vector),y
   adc   #$01
   sta   (vector),y
   iny
   lda   (vector),y
   adc   #$00
   sta   (vector),y
   jmp   output

dec16:
   sec
   ldy   #$00
   lda   (vector),y
   sbc   #$01
   sta   (vector),y
   iny
   lda   (vector),y
   sbc   #$00
   sta   (vector),y
   jmp   output

zero:
   ldx   #$07
:
   stz   hafstart,x
   stz   vafstart,x
   dex
   bpl   :-
   jmp   output

rnd:
   lda   RANDOM
   sta   hafadd+0
   lda   RANDOM
   and   #$0f
   sta   hafadd+1

   lda   RANDOM
   sta   hbfadd+0
   lda   RANDOM
   ora   #$f0
   sta   hbfadd+1

   lda   RANDOM
   sta   haadd+0
   lda   RANDOM
   and   #$07
   sta   haadd+1

   lda   RANDOM
   sta   hbadd+0
   lda   RANDOM
   ora   #$f8
   sta   hbadd+1

   lda   RANDOM
   sta   vafadd+0
   lda   RANDOM
   and   #$0f
   sta   vafadd+1

   lda   RANDOM
   sta   vbfadd+0
   lda   RANDOM
   ora   #$f0
   sta   vbfadd+1

   lda   RANDOM
   sta   vaadd+0
   lda   RANDOM
   and   #$07
   sta   vaadd+1

   lda   RANDOM
   sta   vbadd+0
   lda   RANDOM
   ora   #$f8
   sta   vbadd+1

   jmp   output

irqhandler:
   pha
   phx
   phy

   jsr   plasma_calc
   lda   TMIMRL         ; acknoledge timer

   ply
   plx
   pla
   rti


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
