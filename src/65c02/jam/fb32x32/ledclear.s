
.include "fb32x32.inc"

vector0 := $10
vector1 := $12

FRAMEBUFFER := $cc00

start:
   lda   #<FRAMEBUFFER
   ldx   #>FRAMEBUFFER
   ldy   #$01
   jmp   ($fffc)
