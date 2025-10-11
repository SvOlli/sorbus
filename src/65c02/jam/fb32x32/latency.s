
.include "fb32x32.inc"

; 400 = 40.0ms = 25fps
.define DELAY        400
.define FRAMEBUFFER  $cc00

.segment "CODE"

start:
   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$00
   int   FB32X32

   sei
   lda   #<irq
   sta   UVNBI+0
   lda   #>irq
   sta   UVNBI+1
   lda   #<DELAY
   sta   TM_IMR+0
   lda   #>DELAY
   sta   TM_IMR+1
   cli
:
   jsr   CHRIN
   cmp   #$03
   bne   :-
   stz   TM_IMR+0
   stz   TM_IMR+1
   int   MONITOR
   bra   start

irq:
   pha
   phx
   phy
   ldx   #$00
   lda   #$0f              ; blue
:
   sta   FRAMEBUFFER+$000,x
   sta   FRAMEBUFFER+$100,x
   sta   FRAMEBUFFER+$200,x
   sta   FRAMEBUFFER+$300,x
   inx
   bne   :-
   stz   FB32X32_COPY
@first:
   stz   FRAMEBUFFER+$3ff  ; + 3: will be changed during runtime
   stz   FRAMEBUFFER+$3ff  ; + 6: will be black block at bottom
   stz   FRAMEBUFFER+$3fe  ; + 9: will be black block at bottom
   stz   FRAMEBUFFER+$3fd  ; +12: will be black block at bottom
   stz   FRAMEBUFFER+$3fc  ; +15: will be black block at bottom
   stz   FRAMEBUFFER+$3fb  ; +18: will be black block at bottom
   stz   FRAMEBUFFER+$3fa  ; +21: will NOT be black block at bottom
   dec   @first+1
   lda   @first+1
   cmp   #$ff
   bne   :++
   lda   @first+2
   dec
   cmp   #$cb
   bne   :+
   lda   #$cf
:
   sta   @first+2
:
   bit   TM_IMR            ; acknoledge timer irq
   ply
   plx
   pla
   rti
