
.include "../jam_bios.inc"
.include "../jam.inc"

.import init_decruncher
.import get_decrunched_chunk
.importzp zp_dest_hi
.importzp decrunch_table_end

.export get_crunched_byte
.export buffer_start_hi
.export buffer_len_hi
.export decrunched_chunk_size

exosrc   := decrunch_table_end
rows     := exosrc+2

buffer_start_hi         := $e000
buffer_len_hi           := $1000 ; only 4k work stable @ $e000
decrunched_chunk_size   := buffer_len_hi

.segment "CODE"

start:
   int   COPYBIOS
   stz   BANK
   ldy   #VT100_SCRN_SIZ
   int   VT100
   dec                  ; remove one line for "press SPACE" message
   sta   rows
   cpx   #80
   bcc   toosmall
   cmp   #19            ; 20-1 "dec"
   bcs   menu
toosmall:
   jsr   PRINT
   .byte 10,"Sorry, this program requires a minimum terminal size of 80x20",10,0
   jmp   ($fffc)

menu:
   ldy   #VT100_SCRN_CL0
   int   VT100

   lda   #<WELCOME
   sta   exosrc+0
   lda   #>WELCOME
   sta   exosrc+1
   jsr   runpager

   jsr   PRINT
   .byte 10,"Select document to view (CTRL+C to quit): ",0

input:
   int   CHRINUC
   ldx   #<(txttable-keys-1)
   cmp   #$03
   bne   :+
   jmp   ($fffc)
:
   cmp   keys,x
   beq   @found
   dex
   bpl   :-
   bra   input

@found:
   jsr   CHROUT
   lda   #$0a
   jsr   CHROUT
   txa
   asl
   tax
   lda   txttable+0,x
   sta   exosrc+0
   lda   txttable+1,x
   sta   exosrc+1

   lda   #$0a
   jsr   CHROUT

   jsr   runpager
   bcs   menu

   txa                  ; cpx #$00
   beq   :++
   lda   #$0A
:
   jsr   CHROUT
   dex
   bne   :-
:
   jsr   PRINT
   .byte "  press Q for menu",13,0
:
   int   CHRINUC
   cmp   #$03
   beq   :+
   cmp   #'Q'
   bne   :-
:
   jmp   menu

runpager:
   jsr   init_decruncher
   ldx   rows
@nextchunk:
   phx                  ; get_decrunched_chunk changes X
   jsr   get_decrunched_chunk
   plx
   ldy   #$00           ; reset pointer in current chunk
@printchar:
   dey
   lda   (zp_dest_hi-1),y
   beq   @done          ; $00: end of text (crunched in)
   jsr   CHROUT
   cmp   #$0a           ; is it newline?
   bne   @skip
; do pager stuff here
   dex                  ; decrement # lines before wait space
   bne   @skip          ; number of lines reached?
   jsr   PRINT
   .byte "  press SPACE for next page, Q for menu",13,0
:
   int   CHRINUC
   cmp   #$03
   beq   @quit
   cmp   #'Q'
   beq   @quit
   cmp   #' '
   bne   :-

   phy
   ldy   #VT100_EOLN_CLR
   int   VT100
   ply

   ldx   rows           ; restore number of rows to display
   dex                  ; remove one for last line of previous page
@skip:
   tya                  ; cpy #$00
   bne   @printchar
   beq   @nextchunk

@done:
   clc
   rts
@quit:
   sec
   rts

; -------------------------------------------------------------------
; The decruncher jsr:s to the get_crunched_byte address when it wants to
; read a crunched byte. This subroutine has to preserve x and y register
; and must not modify the state of the carry flag.
; -------------------------------------------------------------------
get_crunched_byte:
   lda   (exosrc)
   inc   exosrc+0
   bne   :+
   inc   exosrc+1
:
   rts


.segment "DATA"

keys:
   .byte "234567"
   .byte "CDFRS"
   .byte "ABMPT"

txttable:
   .word GPLV2
   .word GPLV3
   .word CC_BYNCSA3
   .word BSD0
   .word BSD2
   .word BSD3

   .word C_FLOD
   .word DHARA
   .word FATFS
   .word RESID
   .word PICOSDK

   .word APPLE
   .word MSBASIC
   .word MOS
   .word CPM65
   .word TALIFORTH2

WELCOME:
   .incbin "welcome.exo"

BSD0:
   .incbin "bsd-0clause.exo"
BSD2:
   .incbin "bsd-2clause.exo"
BSD3:
   .incbin "bsd-3clause.exo"
GPLV2:
   .incbin "gplv2.exo"
GPLV3:
   .incbin "gplv3.exo"
CC_BYNCSA3:
   .incbin "cc-by-nc-sa-3.exo"

C_FLOD:
   .incbin "c-flod.exo"
DHARA:
   .incbin "dhara.exo"
FATFS:
   .incbin "fatfs.exo"
RESID:
   .incbin "resid.exo"
PICOSDK:
   .incbin "picosdk.exo"

APPLE:
   .incbin "apple.exo"
MSBASIC:
   .incbin "msbasic.exo"
MOS:
   .incbin "mos.exo"
CPM65:
   .incbin "cpm65.exo"
TALIFORTH2:
   .incbin "taliforth2.exo"
