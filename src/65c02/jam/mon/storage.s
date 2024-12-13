
.include "jam_bios.inc"

.export     directory
.export     loadfile
.export     savefile

.import     getaddr
.import     gethex4
.import     skipspace
.import     prterr
.import     prtnl
.import     newenter
.import     iofailed

.import     INBUF
.importzp   MODE
.importzp   FORMAT
.importzp   LENGTH

.ifp02
.error "this code will never work with NMOS6502"
.endif

; format of command line is
; S<optional_space>0<mandatory_space>filename.ext<mandatory_space>0400<optional_space>1000
; L<optional_space>0<mandatory_space>filename.ext<mandatory_space>0400
; clc -> parse load; sec -> parse save
parsefilename:
   ; A is either "S" or "L"
   asl
   cmp   #$a0           ; A is now $a6 or $98 -> carry set for S only
   ror                  ; A is now $b3 for save or $4c for load
   sta   MODE           ; so now bit MODE bmi can be used to detect save
   stz   FORMAT         ; if FORMAT > $00 it overrides X for error pos
   jsr   skipspace
   jsr   gethex4
   bcs   error
   sta   LENGTH         ; use LENGTH for user

   jsr   checkspace
   stx   FORMAT         ; FORMAT now carries the start offset of filename

:
   lda   INBUF,x
   beq   errorX
   inx
   cmp   #' '
   bne   :-

   dex
   stz   INBUF,x        ; for CPMNAME, we need it $00-terminated
   phx
   lda   FORMAT         ; get start offset for CPMNAME
   ldx   #>INBUF
.if 0
   clc
   adc   #<INBUF
   bcc   :+
   inx
:
.else
.assert (<INBUF) = 0, error, "INBUF must be at pagestart or change .if 0 above"
.endif
   ldy   LENGTH
   int   CPMNAME
   plx
   lda   #' '
   sta   INBUF,x        ; restore space and end of filename
   bcs   error
   stz   FORMAT

   jsr   getaddr        ; get start address
   bcs   error
   sta   CPM_SADDR+0    ; save start address
   sty   CPM_SADDR+1

   bit   MODE
   bpl   :+             ; skip end address for load

   jsr   getaddr        ; get end address
   bcs   error
   sta   CPM_EADDR+0    ; save start address
   sty   CPM_EADDR+1
:
   rts

error:
   lda   FORMAT
   beq   errorX
   tax
errorX:
   jmp   prterr

checkspace:
   lda   INBUF,x
   cmp   #' '
   bne   :+             ; no -> fail
skipspace0:
   jmp   skipspace      ; yes -> skip it
:
   pla
   pla
   bra   error

directory:
   jsr   gethex4
   bcs   error
   tay
   stz   CPM_SADDR+1
   jsr   prtnl
   int   CPMDIR
   rts

loadfile:
   jsr   parsefilename
   int   CPMLOAD
   jsr   PRINT
   .byte 10,"load",0
   bcc   printfromto
   jmp   iofailed

savefile:
   jsr   parsefilename
   int   CPMSAVE
   jsr   PRINT
   .byte 10,"save",0
   bcc   printfromto
   jmp   iofailed

printfromto:
   jsr   PRINT
   .byte " from ",0

   lda   CPM_SADDR+0
   ldx   CPM_SADDR+1
   int   PRHEX16

   jsr   PRINT
   .byte " to ",0

   lda   CPM_EADDR+0
   ldx   CPM_EADDR+1
   int   PRHEX16
   rts
