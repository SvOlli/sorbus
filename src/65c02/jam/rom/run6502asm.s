
.include "fb32x32.inc"

.define     DELAY       500
.export     run6502asm

run6502asm:
   lda   #FB32X32_CMAP_C64
   sta   FB32X32_COLMAP

   lda   #<run6502asm_irq
   sta   UVNBI+0
   lda   #>run6502asm_irq
   sta   UVNBI+1
   lda   #<DELAY ; 20 fps
   sta   TM_IMR+0
   lda   #>DELAY ; 20 fps
   sta   TM_IMR+1
   cli
   jmp   $0600             ; typical start of executable
                           ; others are not supported

run6502asm_irq:
   pha
   phx
   phy

   bit   TM_IMR            ; clear timer IRQ

   lda   #$02              ; framebuffer @ $0200
   stz   FB32X32_SRC+0
   sta   FB32X32_SRC+1

   stz   FB32X32_DEST_X    ; full screen starts at 0
   stz   FB32X32_DEST_Y

   lda   #$1f              ; and has a full size
   sta   FB32X32_WIDTH
   sta   FB32X32_HEIGHT
   sta   FB32X32_STEP

   stz   FB32X32_COPY

   inc   $fc               ; extra feature: frame counter
   bne   :+
   inc   $fd
:
   lda   RANDOM            ; fake registers of 6502asm
   sta   $fe
   jsr   CHRIN
   sta   $ff

   ply
   plx
   pla
   rti
