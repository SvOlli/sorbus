
.include "fb32x32.inc"

.export     emu6502asm

emu6502asm:
   lda   #FB32X32_CMAP_C64
   sta   FB32X32_COLMAP
   lda   #<emu6502asm_irq
   sta   UVNBI+0
   lda   #>emu6502asm_irq
   sta   UVNBI+1
   lda   #<50000 ; 20 fps
   sta   TM_IMR+0
   lda   #>50000 ; 20 fps
   sta   TM_IMR+1
   cli
   jmp   $0600

emu6502asm_irq:
   pha
   phx
   phy

   bit   TM_IMR

   lda   #$02
   stz   FB32X32_SRC+0
   sta   FB32X32_SRC+1

   stz   FB32X32_DEST_X
   stz   FB32X32_DEST_Y

   lda   #$1f
   sta   FB32X32_WIDTH
   sta   FB32X32_HEIGHT
   sta   FB32X32_STEP

   stz   FB32X32_COPY

   inc   $fd
   bne   :+
   inc   $fc
:
   lda   RANDOM
   sta   $fe
   jsr   CHRIN
   sta   $ff
   ply
   plx
   pla
   rti
