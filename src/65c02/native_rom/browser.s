
.include "../native.inc"
.include "../native_bios.inc"
.include "../native_cpmfs.inc"

.segment "CODE"

readp    = $20
endp     = $22
findp    = $24
index    = $26
lastidx  = $27
stoppg   = $28
user     = $29
size     = $2a
cpm_dirty= $2a          ; can be shared

FIRSTLINE   := $03      ; start at 3rd line on screen with filenames
INPUTLINE   := $13
DIRSTART    := $0400    ; must be on page boundry
SX4START    := $0400
FNBUFFER    := $0380
OLDFILE     := FNBUFFER+$10
IOBUFFER    := $DF80

; TODO
; - copy
; - type
; - hexdump

   jmp   (@jmptab,x)

@jmptab:
   .word init

; list of chars not allowed in a filename
; (space is used for padding at the end)
badfnchars:
   .byte " */:<=>?[]"
badfnchars_size = * - badfnchars


setline:
   ldx   #$01
   ldy   #VT100_CPOS_SET
   int   VT100
   rts

clrline:
   jsr   setline
   ldy   #VT100_EOLN_CLR
   int   VT100
   rts

rstclr:
   ldy   #VT100_CPOS_RST
   int   VT100

   ldy   #VT100_EOLN_CLR
   int   VT100

   rts


hexin:
   int   CHRINUC
   cmp   #$03
   bne   :+

   ;sec
   rts
:
   jsr   ishex
   bcs   hexin
   rts


ishex:
   cmp   #'0'
   bcc   @not
   cmp   #'9'+1
   bcc   @is
   cmp   #'A'
   bcc   @not
   cmp   #'F'+1
   bcs   @not
   sbc   #$06
@is:
   and   #$0f
   clc
   rts
@not:
   sec
   rts


; load next directory sector
; and if the current one was modified, save it first
readbuffer:
   lda   cpm_dirty      ; check if current dirbuffer has been modified
   beq   @nowrite

   lda   ID_LBA+0       ; buffer was modified, to save go back one sector
   bne   :+
   dec   ID_LBA+1
:
   dec   ID_LBA+0
   ldx   #$01           ; set I/O offset to write
   jsr   @write         ; save modified buffer

@nowrite:
   ldx   #$00           ; set I/O offset to read
@write:
   lda   #<IOBUFFER
   sta   ID_MEM+0
   lda   #>IOBUFFER
   sta   ID_MEM+1

   stz   IDREAD,x       ; read (X=0) or write (X=1)
   stz   cpm_dirty      ; now the buffer has to be clean
   rts


idx2fname:
   lda   index
   asl
   asl
   asl
   asl
   sta   readp

   ldy   #$0b
@eraloop:
   lda   (readp),y
   sta   OLDFILE,y
   sta   CPM_FNAME,y
   dey
   bpl   @eraloop
   rts


checkentry:
   lda   #$01
   stz   ID_LBA+0
   sta   ID_LBA+1

@dirloop:
   jsr   readbuffer

   lda   #<IOBUFFER
   sta   findp+0
   lda   #>IOBUFFER
   sta   findp+1

@checkfile:
   ldy   #$0b
:
   lda   (findp),y
   cmp   CPM_FNAME,y
   bne   @nextfile
   dey
   bpl   :-
   clc
   rts

@nextfile:
   lda   findp+0
   clc
   adc   #$10
   sta   findp+0

   bmi   @checkfile

   lda   ID_LBA+0
   bne   @dirloop

   sec
   rts


; expects the new filename at CPM_FNAME, and the old filename at OLDFILE
changeentry:
   jsr   checkentry     ; check if target entry exists
   bcs   :+
   rts                  ; entry exists, do nothing
:
   lda   #$01
   stz   ID_LBA+0
   sta   ID_LBA+1
   stz   cpm_dirty

@dirloop:
   jsr   readbuffer

   lda   #<IOBUFFER
   sta   findp+0
   lda   #>IOBUFFER
   sta   findp+1

@checkfile:
   ldy   #$0b
:
   lda   (findp),y
   cmp   OLDFILE,y
   bne   @nextfile
   dey
   bpl   :-

; match: replace file info
   ldy   #$0b
:
   lda   CPM_FNAME,y
   sta   (findp),y
   dey
   bpl   :-
   sty   cpm_dirty

@nextfile:
   lda   findp+0
   clc
   adc   #$10
   sta   findp+0

   bmi   @checkfile

   lda   ID_LBA+0
   bne   @dirloop

   jmp   readbuffer


init:
   ldx   #$00
:
   stz   readp,x        ; clear out used zero page memory
   inx
   bpl   :-

   lda   #$0a           ; use application partition to start
   sta   user

   lda   #$fe           ; set cursor to max bottom right
   tax
   ldy   #VT100_CPOS_SET
   int   VT100
   ldy   #VT100_CPOS_GET
   int   VT100          ; now get cursor position to get screen size
                        ; -> X: columns of screen
                        ; -> A: rows of screen

   cmp   #$16
   bcc   @toosmall
   cpx   #$48
   bcs   :+

@toosmall:
   jsr   PRINT
   .byte " Screen needs to be at least 72x22",0
   jmp   ($fffc)

:
   lda   #$01
   jsr   setline
   
   ldy   #VT100_SCRN_CLR
   int   VT100

   jsr   PRINT
   .byte "Sorbus Filebrowser V0.1   User:    HW-Rev:",0

   ; TRAP contains hardware information. Format:
   ; ASCII of "SBC23" (only hardware code so far), followed by a byte < $20
   ; then a binary sub-versioning ending with $00
:
   lda   TRAP
   bne   :-             ; read until $00

:
   lda   TRAP
   cmp   #' '           ; check for end of ASCII part
   bcc   :+
   jsr   CHROUT         ; print out ASCII part
   bra   :-
:
   pha
   lda   #' '
   jsr   CHROUT
   pla
   int   PRHEX8         ; print the rest as space separated hexdump
   lda   TRAP
   bne   :-

   lda   #INPUTLINE
   jsr   clrline

   jsr   PRINT
   .byte 10
   .byte "<- ->: switch pages"
   .byte " | 0-9 A-F: User"
   .byte " | L: Load"
   .byte 10
   .byte ">: Delete | K: Copy | M: Move"
.if 0
   .byte " | T: Transfer"
.endif
   .byte 0

reload:
   jsr   loaddir
   jmp   browsedir

loaddir:
   lda   #>DIRSTART     ; setup memory start address of $0400
   stz   CPM_SADDR+0
   sta   CPM_SADDR+1
   stz   readp+0
   sta   readp+1
   stz   endp+0
   sta   endp+1

   ldy   user
   int   CPMDIR         ; use interrupt to load directory into RAM

   lda   CPM_SADDR+0    ; check if any entry was loaded
   cmp   CPM_EADDR+0
   bne   @loop
   lda   CPM_SADDR+1
   cmp   CPM_EADDR+1
   bne   @loop

   ldy   #$0f           ; no entry, create a dummy stating "NO FILES"
:
   lda   @dummyentry,y
   sta   (readp),y
   dey
   bpl   :-

   lda   #$10
   sta   endp+0
   jmp   @endpage

@dummyentry:
   .byte $FF,"NO FILES!!!",0,0,2,0

@loop:                  ; loop through all of the entries
   ldy   #$0c
   lda   (readp),y      ; check it it's an additional entry of file
   beq   @noextent      ; it's not, skip some calculation

   stz   findp+0        ; reset pointer for finding original entry
   lda   #>DIRSTART
   sta   findp+1

@checkfile:
   ldy   #$0b
@checkname:
   lda   (findp),y      ; check if it's the same file
   cmp   (readp),y
   bne   @notfound      ; it's not, skip to next entry
   dey
   bpl   @checkname

   ldy   #$0f           ; it is, so add
   clc
   lda   (readp),y      ; add up used sectors lo
   adc   (findp),y
   sta   (findp),y
   dey
   lda   (readp),y      ; add up used sectors hi
   adc   (findp),y
   sta   (findp),y
   dey
   lda   (readp),y      ; copy number of bytes in final sector
   sta   (findp),y
   bra   @next

@notfound:
   clc                  ; move secondary pointer to next entry
   lda   findp+0
   adc   #$10
   sta   findp+0
   bcc   :+
   inc   findp+1
:
   ;lda   findp+0        ; obsolete
   cmp   CPM_EADDR+0    ; check if secondary pointer has reached end
   bne   @checkfile
   lda   findp+1
   cmp   CPM_EADDR+1
   bne   @checkfile

   bra   @next          ; just process next entry

@noextent:
   ldy   #$0f           ; no file extent, copy filename
:
   lda   (readp),y
   sta   (endp),y
   dey
   bpl   :-

   clc                  ; adjust target pointer
   lda   endp+0
   adc   #$10
   sta   endp+0
   bcc   :+
   inc   endp+1
:

@next:
   clc                  ; adjust source pointer
   lda   readp+0
   adc   #$10
   sta   readp+0
   bcc   :+
   inc   readp+1
:
   lda   readp+0        ; check if end is reached
   cmp   CPM_EADDR+0
   bne   @loop
   lda   readp+1
   cmp   CPM_EADDR+1
   bne   @loop

@endpage:
   ldy   #$00           ; clear out rest of current page
   lda   #$e5           ; plus some more to minimize codesize
:
   sta   (endp),y
   iny
   bne   :-

   ldx   endp+1         ; figure of the page of the last entry + 1
   lda   endp+0
   beq   :+
   inx
:
   stx   stoppg         ; store it for user interface

   rts


browsedir:
   lda   #$01
   ldx   #$20
   ldy   #VT100_CPOS_SET
   int   VT100
   lda   user           ; load user partition
   ora   #'0'           ; convert it to hex
   cmp   #'9'+1
   bcc   :+
   adc   #$06
:
   jsr   CHROUT         ; and write it on screen

   lda   #>DIRSTART     ; set to start of page
   stz   readp+0
   sta   readp+1

@prpage:
   lda   #$10           ; up to 16 entries per page can be displayed
   sta   lastidx
   stz   index          ; set arrow to first entry

@prent:
   lda   readp+0        ; use the address
   lsr
   lsr
   lsr
   lsr                  ; shift it to calculate the line on the display
   adc   #FIRSTLINE
   jsr   clrline

   jsr   PRINT
   .byte "    ",0       ; clear out any previous arrow

   ldy   #$01
@prname:
   lda   (readp),y
   beq   @nextent
   cmp   #$e5
   beq   @noent
   and   #$7f
   jsr   CHROUT         ; print out filename
   cpy   #$08           ; after the 8th character
   bne   :+
   lda   #'.'
   jsr   CHROUT         ; print out the dot to separate extension
:
   iny
   cpy   #$0c
   bcc   @prname

   jsr   PRINT
   .byte "    ",0

   ; size calculation need some explanation
.if 1
   ldy   #$0d           ; offset $0d contains bytes in final sector
   lda   #$80           ; calculate how many bytes to strip off
   sec
   sbc   (readp),y
   bpl   :+
   lda   #$00           ; $00 (calculated as $80) means nothing to strip off
:
   sta   size           ; store for later use
   iny                  ; offset $0e contains hi of number of sectors
   lda   (readp),y
   lsr                  ; move 9th bit to carry
   bne   @toolarge      ; if still something is left, filesize > $ffff
   iny                  ; offset $0f contains lo of number of sectors
   lda   (readp),y
   ror                  ; multiply by 128 by dividing by 2 and use lo as hi
   tax
   lda   #$00
   ror                  ; shift in bit7
   sec
   sbc   size           ; now scrape off bytes of final sector
   bcs   :+
   dex
:

.else
   ldy   #$0d           ; offset $0d contains bytes in final sector
   lda   (readp),y
   bne   :+
   lda   #$80           ; $00 means $80 bytes, so adjust that
:
   sta   size           ; store for later use
   iny                  ; offset $0e contains hi of number of sectors
   lda   (readp),y
   lsr                  ; move 9th bit to carry
   bne   @toolarge      ; if still something is left, filesize > $ffff
   iny                  ; offset $0f contains lo of number of sectors
   lda   (readp),y
   beq   :+             ; $100 will get decremented to $0ff
   clc
:
   dec                  ; number of sectors is one too high for calculation
   ror                  ; now multiply by 128 by dividing by 2 and use lo as hi
   tax
   lda   #$00           ; clear out lo
   ror                  ; shift in carry
   adc   size           ; add bytes used in final sector
   bcc   :+
   inx                  ; add carry
:
.endif
   int   PRHEX16
   ; Due to logic involved a filesize of extactly 64k is calculated as $0000
   ; since a zero byte file is not possible, let's keep it in and call it a
   ; feature.

   bra   @nextent

@toolarge:
   jsr   PRINT
   .byte "****",0
   bra   @nextent

@noent:
   dec   lastidx

   ldy   #VT100_EOLN_CLR
   int   VT100

@nextent:
   lda   readp+0
   clc
   adc   #$10
   sta   readp+0
   bne   @prent

@setarrow:
   lda   index
   clc
   adc   #FIRSTLINE-1
   jsr   setline
   jsr   PRINT
   .byte "   ",10," =>",10,"   ",0

   lda   #INPUTLINE
   jsr   clrline

   ldy   #VT100_EOLN_CLR
   int   VT100

@inputloop:
   int   CHRINUC

   jsr   ishex
   bcs   :+
   sta   user
   jsr   loaddir
   jmp   browsedir
:
   cmp   #$03           ; CTRL+C
   beq   @leave
   cmp   #$1b           ; ESC
   beq   @esc
   ldx   DIRSTART       ; check for dummy entry
   bmi   @inputloop

   cmp   #'L'
   bne   :+
   jmp   load
:
   cmp   #'K'
   bne   :+
   jmp   copy
:
   cmp   #'M'
   bne   :+
   jmp   move
:
   cmp   #'>'
   bne   :+
   jsr   idx2fname
   int   CPMERASE
   jmp   reload
:
   bra   @inputloop

@leave:
   ldy   #VT100_SCRN_CLR
   int   VT100
   jmp   ($fffc)

@esc:
   int   CHRINUC
   cmp   #$1b
   beq   @leave
   cmp   #'['
   bne   @inputloop+2

   int   CHRINUC
   cmp   #'A'
   beq   @up
   cmp   #'B'
   beq   @down
   cmp   #'C'
   beq   @right
   cmp   #'D'
   beq   @left
   bne   @inputloop+2

@left:
   ldx   readp+1
   dex
   cpx   #>DIRSTART
   bcc   :+
   stx   readp+1
:
   bra   @prpage2
@right:
   ldx   readp+1
   inx
   cpx   stoppg
   bcs   @prpage2
   stx   readp+1
@prpage2:
   jmp   @prpage
@down:
   ldx   index
   inx
   cpx   lastidx
   bcs   :+
   stx   index
:
   bra   @setarrow2
@up:
   ldx   index
   dex
   bmi   @setarrow2
   stx   index
@setarrow2:
   jmp   @setarrow



move:
   jsr   idx2fname

   jsr   edituserorname
   bcs   @skip

   stz   readp

   jsr   changeentry

@skip:
   jmp   reload


copy:
   ; TODO
   jsr   idx2fname

   lda   OLDFILE+$E     ; check hibyte of #sectors
   beq   @fits
   cmp   #$02
   bcs   @toolarge
   ; A=$01 at this point

   lda   OLDFILE+$F     ; check lobyte of #sectors
   cmp   #$99           ; $0198: maximum #sectors fit in RAM
   bcc   @fits

@fits:
   int   CPMLOAD

   jsr   edituserorname

   bcs   @reload

   int   CPMSAVE
   bra   @reload

@toolarge:
   jsr   PRINT
   .byte "file too large to be copied",0
   int   CHRINUC

@reload:
   jmp   reload


load:
   lda   index
   asl
   asl
   asl
   asl
   inc
   ldx   readp+1

   ldy   user
   int   CPMNAME

   ; do two things in one loop:
   ; 1) check if the filename ends in SX4
   ; 2) copy code for setting bank to $00 before running file
   ldy   #$02
@loadloop:
   lda   CPM_FNAME+$09,y
   cmp   @fileext,y
   beq   :+
   jmp   reload
:
   lda   @bankcode,y
   sta   SX4START-3,y
   dey
   bpl   @loadloop

   int   CPMLOAD
   int   COPYBIOS

   ldy   #VT100_SCRN_CLR
   int   VT100

   jmp   SX4START-3

@bankcode:
   stz   BANK
@fileext:
   .byte "SX4"


editname:
   ; start with saving cursor position
   ldy   #VT100_CPOS_SAV
   int   VT100

   ; copy filename to edit buffer
   ldx   #$00
   ldy   #$00
@copyloop:
   lda   CPM_FNAME+1,y
@copy2:
   sta   FNBUFFER,x
   inx
   cpx   #$08
   bne   :+
   lda   #'.'
   bra   @copy2
:
   iny
   cpy   #$0b
   bcc   @copyloop

   jsr   @findendfn
   
@printfn:
   ; print current filename (without user)
   tya
   ldy   #VT100_CPOS_RST
   int   VT100
   tay

   ldx   #$00
:
   lda   FNBUFFER,x
   jsr   CHROUT
   inx
   cpx   #$0c
   bcc   :-

   ; for loop edit iteration, set cursor to saved position
   tya                  ; save current editing position
   ldy   #VT100_CPOS_RST
   int   VT100
   tay                  ; restore current editing position
   tax                  ; set up counter for moving right
   beq   @keyloop

:
   jsr   PRINT
   .byte $1b,$5b,$43    ; ESC-"[C" --> cursor right
   .byte $00
   dex
   bne   :-

@keyloop:
   int   CHRINUC
   cmp   #$03           ; CTRL-C --> cancel
   bne   :+
   ;sec                 ; implied, cause cmp equal always set carry
   rts
:

   cmp   #$0d           ; RETURN --> done
   bne   :+
   clc
   rts
:

   cmp   #$0c           ; CTRL-L --> redraw
   beq   @printfn

   cmp   #$08           ; backspace
   beq   @del
   cmp   #$7f           ; del
   bne   @nodel
@del:
   cpy   #$00
   beq   @keyloop       ; nothing to erase
   cpy   #$09
   beq   @gofn          ; nothing to erase
   dey                  ; get back to previous character
   lda   #' '
   sta   FNBUFFER,y     ; and set to space
   lda   #$7f           ; erase the character on screen and backstep
   jsr   CHROUT
   bra   @keyloop
@nodel:

   cmp   #'.'
   bne   @nodot

   ; toggle between name and extension
   cpy   #$09
   bcc   :+
@gofn:
   jsr   @findendfn
   bra   @printfn
:
   jsr   @findendfe
   bra   @printfn
@nodot:

   cpy   #$08
   beq   @keyloop
   cpy   #$0c
   bcs   @keyloop

   ldx   #badfnchars_size-1
:
   cmp   badfnchars,x
   beq   @keyloop       ; illegal char --> return to input
   dex
   bpl   :-

   sta   FNBUFFER,y
   iny
   jsr   CHROUT
   bra   @keyloop

@findendfn:
   ldy   #$00           ; end of filename
   lda   #$08
   bra   @findend
@findendfe:
   ldy   #$09
   lda   #$0c
@findend:
   sta   $ff
   lda   #' '           ; space to check for
:
   cmp   FNBUFFER,y
   beq   :+             ; space -> done
   iny                  ; next letter
   cpy   $ff            ; check for end
   bcc   :-             ; not -> repeat
:
   ; Y now contains position
   rts


edituserorname:
   ldy   #VT100_CPOS_SAV
   int   VT100

   jsr   PRINT
   .byte "Name or User? ",0

@keyloop:
   int   CHRINUC
   cmp   #$03
   beq   @cancel

   cmp   #'N'
   bne   :+

   jsr   rstclr
   jsr   PRINT
   .byte "Name: ",0
   
   jsr   editname
   bcs   @cancel

   lda   #<FNBUFFER
   ldx   #>FNBUFFER
   ldy   CPM_FNAME
   int   CPMNAME

   bra   @done

:
   cmp   #'U'
   bne   @keyloop

   jsr   rstclr
   jsr   PRINT
   .byte "User: ",0

   jsr   hexin
   bcs   @cancel

   sta   CPM_FNAME

@done:
   clc
   rts

@cancel:
   sec
   rts
