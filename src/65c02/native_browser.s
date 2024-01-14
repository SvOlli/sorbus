
.include "native.inc"
.include "native_bios.inc"
.include "native_cpmfs.inc"

.segment "CODE"

; this software is in early development
; use bin2hex to generate hex files of this and "writeboot"
; run WozMon
; copy both files into input
; enter "C000R"
; select a partition to write to, "2" is recommended

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
DIRSTART    := $0400    ; must be on page boundry
FNBUFFER    := $0380
OLDFILE     := FNBUFFER+$10
IOBUFFER    := $DF80

; TODO
; - copy
; - type
; - hexdump

   jmp   init           ; start
   .byte "SBC23"        ; boot block id

setline:
   ldx   #$01
   ldy   #VT100_CPOS_SET
   int   VT100
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

; expects the new filename at CPM_FNAME, and the old filename at OLDFILE
changeentry:
   lda   #$01
   stz   ID_LBA+0
   sta   ID_LBA+1
   stz   cpm_dirty

@dirloop:
   jsr   readbuffer

   lda   #$80
   sta   findp+0
   lda   #$DF
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

fnedit:
   lda   index
   asl
   asl
   asl
   asl
   sta   readp
   ldx   #$00
   ldy   #$01

@loop:
   lda   (readp),y
   cmp   #' '+1
   bcc   :+
   sta   FNBUFFER,x
   inx
:
   iny
   cpy   #$09
   bne   :+
   lda   #'.'
   sta   FNBUFFER,x
   inx
:
   cpy   #$0c
   bcc   @loop

   stz   FNBUFFER,x
   stz   readp

   ldx   #$00
@prloop:
   lda   FNBUFFER,x
   beq   @inloop
   jsr   CHROUT
   inx
   bra   @prloop

@inloop:
   int   CHRINUC
   cmp   #$0d           ; RETURN
   beq   @done
   cmp   #$08           ; BS
   beq   @del
   cmp   #$7f
   beq   @del
   cmp   #' '+1
   bcc   @inloop
   ldy   #badfnchars_size-1
:
   cmp   badfnchars,y
   beq   @inloop
   dey
   bpl   :-

   cmp   #'.'
   bne   @nodot

   ldy   #$00
@finddot:
   lda   FNBUFFER,y
   beq   @dodot
   cmp   #'.'
   beq   @inloop
   iny
   bra   @finddot
@dodot:
   tya
   beq   @inloop
   lda   #'.'
@nodot:

   cpx   #$0d
   bcs   @inloop
   sta   FNBUFFER,x
   inx
   stz   FNBUFFER,x
   jsr   CHROUT
   bra   @inloop

@del:
   cpx   #$00
   beq   :+
   dex
   lda   #$7f
   jsr   CHROUT
   stz   FNBUFFER,x
:
   bra   @inloop
@done:
   jmp   loaddir

init:
;   jsr   $0100

   ldx   #$00
:
   stz   readp,x
   inx
   bpl   :-

   lda   #$0a
   sta   user

   ldy   #VT100_SCRN_CLR
   int   VT100

   lda   #$fe
   tax
   ldy   #VT100_CPOS_SET
   int   VT100
   ldy   #VT100_CPOS_GET
   int   VT100

   cmp   #$14
   bcc   @toosmall
   cpx   #$46
   bcs   :+

@toosmall:
   jsr   PRINT
   .byte " Screen needs to be at least 70x20",0
   jmp   ($fffc)

:

   lda   #$01
   jsr   setline
   
   jsr   PRINT
   .byte "Sorbus Browser V0.1   User:",0
   
   lda   #$14
   jsr   setline
   
   jsr   PRINT
   .byte "<- ->: switch pages"
   .byte " | 0-9 A-F: user"
   ;.byte " | K)opy"
   .byte " | N)ame"
   .byte " | U)ser"
   .byte 0

reload:
   jsr   loaddir
   
   jmp   browsedir

loaddir:
   lda   #>DIRSTART
   stz   CPM_SADDR+0
   sta   CPM_SADDR+1
   stz   readp+0
   sta   readp+1
   stz   endp+0
   sta   endp+1

   ldy   user
   int   CPMDIR

   lda   CPM_SADDR+0
   cmp   CPM_EADDR+0
   bne   @loop
   lda   CPM_SADDR+1
   cmp   CPM_EADDR+1
   bne   @loop
   
   ldy   #$0f
:
   lda   @dummyentry,y
   sta   (readp),y
   dey
   bpl   :-

   lda   #$10
   sta   endp+0
   jmp   @endpage
   
@dummyentry:
   .byte $FF,"NO FILES   ",0,0,0,0

@loop:
   ldy   #$0c
   lda   (readp),y
   beq   @noextent

   stz   findp+0
   lda   #>DIRSTART
   sta   findp+1

@checkfile:
   ldy   #$0b
@checkname:
   lda   (findp),y
   cmp   (readp),y
   bne   @notfound
   dey
   bpl   @checkname

   ldy   #$0f
   clc
   lda   (readp),y
   adc   (findp),y
   sta   (findp),y
   dey
   lda   (readp),y
   adc   (findp),y
   sta   (findp),y
   bra   @next

@notfound:
   clc
   lda   findp+0
   adc   #$10
   sta   findp+0
   bcc   :+
   inc   findp+1
:
   lda   findp+0
   cmp   CPM_EADDR+0
   bne   @checkfile
   lda   findp+1
   cmp   CPM_EADDR+1
   bne   @checkfile

   bra   @next

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

   lda   endp+1
   cmp   #>DIRSTART
   bne   :+
   lda   endp+0
   bne   :+
   jmp   ($fffc)

:
   ldx   endp+1
   lda   endp+0
   beq   :+
   inx
:
   stx   stoppg

   rts


browsedir:
   lda   #$01
   ldx   #$1c
   ldy   #VT100_CPOS_SET
   int   VT100
   lda   user
   ora   #'0'
   cmp   #'9'+1
   bcc   :+
   adc   #$06
:
   jsr   CHROUT

   lda   #>DIRSTART
   stz   readp+0
   sta   readp+1

@prpage:
   lda   #$10
   sta   lastidx
   stz   index

@prent:
   lda   readp+0
   lsr
   lsr
   lsr
   lsr
   adc   #FIRSTLINE
   ldx   #$02
   ldy   #VT100_CPOS_SET
   int   VT100

   jsr   PRINT
   .byte "   ",0

   ldy   #$01
@prname:
   lda   (readp),y
   beq   @nextent
   cmp   #$e5
   beq   @noent
   and   #$7f
   jsr   CHROUT
   cpy   #$08
   bne   :+
   lda   #'.'
   jsr   CHROUT
:
   iny
   cpy   #$0c
   bcc   @prname

   lda   #' '
   jsr   CHROUT
   jsr   CHROUT
   ldy   #$0e
   lda   (readp),y
   tax
   iny
   lda   (readp),y
   int   PRHEX16

   bra   @nextent

@noent:
   dec   lastidx
   lda   #' '
:
   jsr   CHROUT
   iny
   cpy   #$13
   bcc   :-

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

   lda   #FIRSTLINE+$10
   jsr   setline

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
   cmp   #'N'
   bne   :+
   jmp   rename
:
   cmp   #'U'
   bne   :+
   jmp   chuser
:
   bra   @inputloop

@leave:
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

chuser:
   jsr   PRINT
   .byte "User: ",0

   int   CHRINUC
   jsr   ishex
   bcs   @skip

   pha
   lda   index
   asl
   asl
   asl
   asl
   sta   readp

   ldy   #$0b
:
   lda   (readp),y
   sta   OLDFILE,y
   sta   CPM_FNAME,y
   dey
   bpl   :-

   pla
   sta   CPM_FNAME
   stz   readp

   jsr   changeentry

@skip:
   jmp   reload

rename:
   jsr   PRINT
   .byte "Rename: ",0

   lda   index
   asl
   asl
   asl
   asl
   sta   readp

   ldy   #$0b
:
   lda   (readp),y
   sta   OLDFILE,y
   dey
   bpl   :-

   jsr   fnedit
   ;stz   readp          ; will be done by fnedit

   lda   FNBUFFER
   beq   @skip

   lda   #<FNBUFFER
   ldx   #>FNBUFFER
   ldy   user
   int   CPMNAME

   jsr   changeentry

@skip:
   jmp   reload


badfnchars:
   .byte "*/:<=>?[]"
badfnchars_size = * - badfnchars
