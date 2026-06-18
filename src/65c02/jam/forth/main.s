
.include "jam.inc"
.include "jam_bios.inc"
.include "jumptable.inc"

.segment "CODE"
CODE_START:

.segment "DATA"
DATA_START:

.segment "ROMSTART"

ROMSTART:
.assert * = B4FORTH, error, "B4FORTH does not match"
   ; bank jumptable index $00
   jmp   load_kern
.assert * = B4AUTORUN, error, "B4AUTORUN does not match"
   ; bank jumptable index $01
   jmp   load_other     ; intended to be used after setting a filename
JTSIZE = * - ROMSTART

.segment "CODE"
load_kern:
   ; int CPMNAME does not work here, filename would be read from wrong bank
   ldx   #$0d
:
   lda   kern_fileinfo,x
   sta   CPM_FNAME,x
   dex
   bpl   :-

load_other:
   jsr   PRINT
   .byte 10,"Loading ",$22,0
   ldx   #$01
@fnloop:
   lda   CPM_FNAME,x
   jsr   CHROUT
   cpx   #$08
   bne   :+
   lda   #'.'
   jsr   CHROUT
:
   inx
   cpx   #$0d
   bcc   @fnloop

   int   CPMLOAD
   bcc   @loadok
   jsr   PRINT
   .byte $22,10,"Failed",10,0
   jmp   ($fffc)

@loadok:
   jsr   PRINT
   .byte $22,10,"Okay",10,0

   jsr   PRINT
   .byte 10,"This bank is reserved for Forth",10,0
   jmp   ($fffc)


.segment "DATA"
kern_fileinfo:
   .byte 4              ; userid
   .byte "KERNEL  FTH"  ; filename (8+3)
   .word $0400          ; start address

.segment "CODE"
CODE_END:

.segment "DATA"
DATA_END:

.out "   ======================"
.out .sprintf( "   FORTH CODE size: $%04x", CODE_END - CODE_START )
.out .sprintf( "   FORTH DATA size: $%04x", DATA_END - DATA_START )
.out .sprintf( "   free ROM       : $%04x", $1F00 - JTSIZE - CODE_END + CODE_START - DATA_END + DATA_START )
.out "   ======================"
