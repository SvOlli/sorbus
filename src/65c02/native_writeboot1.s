
.include "native.inc"

; this code writes the memory from $2000-$3FFF to the bootblock 1
; it is suggested to use bin2hex to transfer the bootblock data to memory

.segment "CODE"
   lda   #$40
   sta   ID_LBA+0
   lda   #$00
   sta   ID_LBA+1
   sta   ID_MEM+0
   lda   #$20
   sta   ID_MEM+1
   ldy   #$40
:
   sta   IDWRT
   dey
   bne   :-

   jmp   $E000
