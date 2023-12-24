
.include "native.inc"
.include "native_bios.inc"

; TODO find a better way to define BIOS calls
CHRIN  = $FF00
CHROUT = $FF03
PRINT  = $FF06

; this code writes the memory from $2000-$3FFF to the bootblock 1
; it is suggested to use bin2hex to transfer the bootblock data to memory

.segment "CODE"
   jsr   PRINT
   .byte 10
   .byte "This program writes the RAM area $E000-$FFFF to a bootblock.", 10
   .byte "Select bootblock to write (1-3): ", 0
:
   jsr   CHRIN
   cmp   #$1B           ; ESC
   beq   @done
   cmp   #'1'
   bcc   :-
   cmp   #'4'
   bcs   :-

   jsr   CHROUT         ; print out successful input, just for fun

   asl                  ; convert bootblock number to starting sector
   asl
   asl
   asl
   asl
   asl

   sta   ID_LBA+0
   lda   #$00
   sta   ID_LBA+1
   sta   ID_MEM+0
   lda   #$E0           ; $E000 is sane, as DMA always works with RAM
   sta   ID_MEM+1

   lda   #10
   jsr   CHROUT

   ldy   #$40           ; number of sectors to write
   lda   #'.'           ; print out a dot for every sector written
:
   sta   IDWRT
   jsr   CHROUT
   dey
   bne   :-

@done:
   lda   #10            ; done, newline
   jsr   CHROUT

   jmp   $E000          ; do something useful: jump back to ROM
