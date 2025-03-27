
.include "../jam_bios.inc"
.include "../jam.inc"

.import init_decruncher
.import get_decrunched_chunk
.importzp zp_dest_hi

.export get_crunched_byte
.export buffer_start_hi
.export buffer_len_hi
.export decrunched_chunk_size

rows     := $1a
cols     := $1b

buffer_start_hi         := $e000
buffer_len_hi           := $1000 ; only 4k work stable @ $e000
decrunched_chunk_size   := buffer_len_hi

.segment "CODE"

start:
   int   COPYBIOS
   stz   BANK
   ldy   #VT100_SCRN_SIZ
   int   VT100
   sta   rows
   stx   cols
   cpx   #80
   bcc   toosmall
   cmp   #20
   bcs   menu
toosmall:
   jsr   PRINT
   .byte 10,"Sorry, this program requires a minimum terminal size of 80x20",10,0
   jmp   ($fffc)

menu:
   ldy   #VT100_SCRN_CL0
   int   VT100

   lda   #<WELCOME
   sta   getraw+1
   lda   #>WELCOME
   sta   getraw+2
   jsr   runpager

   jsr   PRINT
   .byte 10,"Select document to view: ",0

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
   txa
   asl
   tax
   lda   txttable+0,x
   sta   getraw+1
   lda   txttable+1,x
   sta   getraw+2

   lda   #$0a
   jsr   CHROUT

   jsr   runpager
   bcs   menu

   lda   #$0A
   dex
:
   jsr   CHROUT
   dex
   bne   :-

   jsr   PRINT
   .byte 10,"  press Q for menu",13,0
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
   dex
@nextchunk:
   phx
   jsr   get_decrunched_chunk
   plx
   ldy   #$00
@printchar:
   dey
   lda   (zp_dest_hi-1),y
   beq   @done
   jsr   CHROUT
   cmp   #$0a
   bne   @nonl
; do pager stuff here
   dex
   bne   @nonl
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

   ldx   rows
   dex
   dex
@nonl:
   tya
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
   php
getraw:
   lda   $ffff
   inc   getraw+1
   bne   :+
   inc   getraw+2
:
   plp
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
