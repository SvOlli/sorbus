
.include "jam.inc"
.include "jam_bios.inc"

.segment "CODE"
   jsr   PRINT
   .byte 10,"This program writes the RAM area $E000-$FFFF to a bootblock."
   .byte 10,"Typical bootblock use:"
   .byte 10,"1) CP/M kernel"
   .byte 10,"2) NMOS 6502 toolkit"
   .byte 10,"3) unused"
   .byte 10,"4) unused"
   .byte 10,"Select bootblock to write (1-4): ", 0
:
   jsr   CHRIN
   cmp   #$1B           ; ESC
   beq   @done
   cmp   #$03           ; Ctrl+C
   beq   @done
   cmp   #'1'
   bcc   :-
   cmp   #'5'
   bcs   :-

   jsr   CHROUT         ; print out successful input, just for fun

   dec                  ; convert from offset "1" to offset "0"
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
:
   lda   IDWRT
   bpl   :-
   ;bvs   @fail
   jsr   CHROUT
   dey
   bne   :--

@done:
   lda   #10            ; done, newline
   jsr   CHROUT

   jmp   ($FFFC)        ; do something useful: jump back to ROM
