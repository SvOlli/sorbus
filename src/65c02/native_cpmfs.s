
; CP/M filesystem load / save reimplementation

.include "native.inc"
.include "native_bios.inc"
.include "native_kernel.inc"
.include "native_cpmfs.inc"

; cpmfs configuration on Sorbus
; 32768 total sectors
; 2048 bytes per block (16 sectors)
; 1024 directory entries
; 256 boot sectors
; block 0 = 1st directory sector = sector $0100
; if those parameters change, this code needs to be adjusted, big time

; as it loads to RAM (52k continuous memory max), no more than
; 26 (2k) blocks are supported

; extract from cpm.5 manpage (by cpmtools)
; only supported entries are mentioned

; Directory entries:
; St F0 F1 F2 F3 F4 F5 F6 F7 E0 E1 E2 Xl Bc Xh Rc
; Al Al Al Al Al Al Al Al Al Al Al Al Al Al Al Al
;
; St: status
; 0-15: used for file, status is the user number.
; 0xE5: unused
; F0-F7: file name (bits 0-6: ASCII), bit 7 ignored
; E0-E2: extension (bits 0-6: ASCII)
;                  (bit 7 set in E0 or E1 acts as write protection)
; Xl and Xh: store the extent number. (Xh=$00 always)
; Rc and Bc: determine the length of the data used by this extent.
; Bc: stores the number of bytes in the last used record ($00=all)
; Rc: number of sectors specified by entry ($80=scan for next entry)
; Al: stores block pointers (16 bit, lo/hi).
;     block pointers need to be calculated to sectors

; developer notes:
; - block count starts at directory (sector $100)
; - Rc entries can be added up to get total number of blocks
; - Bc*$10 is offset in blocktable

; raw load (save?) structure
; byte 0+1: number of 128 bytes sectors to load (lo/hi)
; block table starts here
; byte 2+3: starting block number of 1st block (lo/hi)
; byte 4+5: starting block number of 2nd block (lo/hi)
; ... up to 26 blocks

; required for saving
; 256 to store a block of 2048 bits indicating which blocks are used
; 1=free, 0=used, that way a lda, bne will show up at least one free block
; $0200-$02ff suggested. The same buffer can be also used when loading
; exomized files


.global  cpm_fname
freetab   = $0200
cpm_fname = $0300       ; format: St F0 F1 F2 F3 F4 F5 F6 F7 E0 E1 E2
cpm_saddr = cpm_fname + $0c ; word: load,save: start address
cpm_eaddr = cpm_fname + $0e ; word: save: end address
cpm_hndlr = cpm_fname + $10 ; word: handler function pointer for scandir
cpm_dirty = cpm_fname + $12 ; byte: save,delete: current dirbuffer needs to be saved
cpm_spare = cpm_fname + $13 ; byte: nothing
cpm_ndent = cpm_fname + $14 ; byte: save: number of dirents to create
cpm_cdent = cpm_fname + $15 ; byte: save: number of dirents created
cpm_nblck = cpm_fname + $16 ; byte: number of blocks (end of lblck)
cpm_cblck = cpm_fname + $17 ; byte: current block (pos in lblck)
cpm_nsect = cpm_fname + $18 ; word: load,save: number of sectors (save: left)
cpm_lblck = cpm_fname + $1a ; wordlist: load,save: block list for file
cpm_blend = cpm_lblck + $40 ; end of memory used, should be $035a

buffer    = $df80        ; scratchbuffer
tmp16     = $fe          ; also used by jsr PRINT

.define  DEBUG 0

;-------------------------------------------------------------------------
; this file provides four functions
; cpmname requires the following input
; A: lo-pointer to "FILENAME.EXT"
; X: hi-pointer to "FILENAME.EXT"
; Y: user partition ($00-$0f)
; it converts the "FILENAME.EXT" to the format required in cpm_fname
;
; all following functions require the filename at cpm_fname in format:
; St F0 F1 F2 F3 F4 F5 F6 F7 E0 E1 E2
; cpmload requires start address in cpm_saddr
; cpmsave requires start and end+1 addresses in cpm_saddr and cpm_eaddr
; cpmerase requires no addresses
; load and save work on full 128 bytes block chunks !!!
;
; TODO: directory function
; directory function requires load address to be set (data can be 16kB max)
; if load address is set to <$0100, it will be printed to screen
; format in RAM: St F0 F1 F2 F3 F4 F5 F6 F7 E0 E1 E2 Xl Bc Xh Rc
; format on screen: F0 F1 F2 F3 F4 F5 F6 F7 (dot) E0 E1 E2 (four spaces)
;-------------------------------------------------------------------------
; NOTE: these functions were implemented trying to use a minimum of ROM
; space. Therefore there will be only a minimum of error checking and
; handling. Also, it should be easier to speed things up. Still ROM size
; was the main focus and it's still fast enough.
;-------------------------------------------------------------------------

;-------------------------------------------------------------------------
; API functions
;-------------------------------------------------------------------------

cpmname:
   sta   tmp16+0        ; save source vector
   stx   tmp16+1
   sty   cpm_fname      ; save user

   lda   #' '           ; fill cpm_fname with spaces
   ldy   #$0b
:
   sta   cpm_fname,y
   dey
   bne   :-

   ;ldy   #$00           ; implicit from loop
   ldx   #$01           ; cpm_fname stored with offset 1
@loop:
   lda   (tmp16),y      ; load char of cpm_fname
   beq   @done          ; $00 - end of file
   and   #$7f           ; comparison is on ASCII only
   jsr   uppercase      ; and also uppercase
:
   cmp   #'.'
   bne   @nodot
   ldx   #$08
   bne   @skip
@nodot:
   sta   cpm_fname,x
@skip:
   iny
   inx
   cpx   #$0c
   bcc   @loop
@done:
cpmexiterr:
   sec
   rts

cpmload:
   stz   cpm_nsect+0
   stz   cpm_nsect+1

   ldx   #<i_getlblks
   jsr   scandir

   beq   cpmexiterr
   ldx   #$00           ; select read mode
   jsr   dmafile

   lda   ID_MEM+0
   sta   cpm_eaddr+0
   lda   ID_MEM+1
   sta   cpm_eaddr+1
   rts

cpmsave:
   jsr   cpmerase       ; in order to replace, delete first
   jsr   setupsave
   bcs   cpmexiterr
   ldx   #$01           ; select write mode
   jmp   dmafile

cpmdir:
   sty   cpm_fname

   lda   cpm_saddr+0
   sta   tmp16+0
   lda   cpm_saddr+1
   sta   tmp16+1

   ldx   #<i_getdir
   jsr   scandir

   lda   tmp16+1
   beq   :+
.if 0
   lda   #$e5           ; write end of dir marker
   sta   (tmp16)
.else
   lda   tmp16+0        ; write end address to cpm_eaddr
   sta   cpm_eaddr+0
   lda   tmp16+1
   sta   cpm_eaddr+1
.endif
   rts
:
   lda   #$0a
   jmp   CHROUT

cpmerase:
   ldx   #<i_erase
   ; slip through to scandir

;-------------------------------------------------------------------------
; internal functions
;-------------------------------------------------------------------------

scandir:
   lda   dirhandlers+0,x
   sta   cpm_hndlr+0
   lda   dirhandlers+1,x
   sta   cpm_hndlr+1

   stz   ID_LBA+0
   lda   #$01
   sta   ID_LBA+1       ; directory starts at LBA=$0100 (ends at $0200)

@nextblock:
   jsr   readbuffer

   ;ldx   #$00           ; readbuffer returns X=0
   ;stz   deletemarker   ; deletemarker also cleared by readbuffer
@nextentry:
   ldy   #$00           ; every handler requires Y=0
   lda   buffer,x
   cmp   #$e5

   ; handle it
   ; Y=$00 (required by all dirhandlers)
   ; X=$00,$20,$40,$60 (start of direntry)
   ; A=first byte of entry (type)
   ; Z=1 -> deleted entry
   jsr   jmphandler

   txa
   ora   #$1f
   inc
   tax
   bpl   @nextentry
   lda   ID_LBA+0       ; pointer gets autoincremented by DMA
                        ; directory is 1024 entries
                        ; (4 entries per block = 256 blocks)
   bne   @nextblock
   jsr   readbuffer     ; write back dirty buffer
   lda   cpm_nsect+0
   ora   cpm_nsect+1    ; Z=1 nothing found
   rts

dmafile:
   ; DMA transfer the file, X=0: load, X=1: save
   lda   cpm_saddr+0    ; copy start address
   sta   ID_MEM+0       ; to DMA
   lda   cpm_saddr+1
   sta   ID_MEM+1

   stz   tmp16+0        ; reset current sector
   stz   tmp16+1

@loop:
   lda   tmp16+0
   and   #$0f
   bne   @nonewblock

   lda   tmp16+1        ; get next block address from buffer
   lsr                  ; since blockcount is not >= $0200 (64k of data)
                        ; only 1 bit needs to be shifted in

   lda   tmp16+0        ; calculate offset in block table
   ror                  ; divide by 8 and strip off lowest bit
   lsr
   lsr
   and   #$fe
   tay                  ; range is now 0,2,...,30

   ; Y contains block index in blkls
   lda   cpm_lblck+0,y
   sta   ID_LBA+0
   lda   cpm_lblck+1,y
   ldy   #$04           ; since there are 16 sectors in a block do 4 shift lefts
:
   asl   ID_LBA+0
   rol
   dey
   bne   :-
   inc                  ; now add the one boottrack
   sta   ID_LBA+1

@nonewblock:
   stz   IDREAD,x       ; in save X=1 to trigger write instead of load

   inc   tmp16+0        ; increment sector number
   bne   :+
   inc   tmp16+1
:

   lda   tmp16+0        ; check if number of sectors is reached
   cmp   cpm_nsect+0
   bne   @loop
   lda   tmp16+1
   cmp   cpm_nsect+1
   bne   @loop
   clc                  ; C=0: all okay
   rts


;-------------------------------------------------------------------------
; saving
;-------------------------------------------------------------------------

; this function does quite a lot
; - calculating
;   - cpm_nsect (number of 128 bytes sectors to be saved)
;   - cpm_nblck (number of 2k blocks required)
;   - cpm_ndent (number of directory entries required)
; - finding free blocks
;   - write them to cpm_lblck
; - create directory entries

setupsave:
   clc                  ; subtract one more, assuming end address is + 1
   lda   cpm_eaddr+0
   sbc   cpm_saddr+0
   tax
   lda   cpm_eaddr+1
   sbc   cpm_saddr+1
   tay

   ; Y,X contain filesize hi/lo
   txa                  ; want to divide by 128
   asl                  ; just need highest save highest bit in carry
   tya                  ; now get hibyte
   rol                  ; multiply by 2
   sta   cpm_nsect+0    ; saving in lo is like divide by 256
   sta   tmp16+0
   lda   #$00           ; now carry contains 9th bit
   rol                  ; shift that into hi byte
   sta   cpm_nsect+1    ; number of sectors saved
   sta   tmp16+1

   inc   cpm_nsect+0    ; add one for accurate count
   bne   :+
   inc   cpm_nsect+1
:

   ; maximum number of sectors:
   ; 52k ($34k) bytes -> 416 ($1a0) sectors -> 26 ($1a) blocks
   ; expected results:
   ; #sectors | #blocks | #dirents
   ;    1- 16 |       1 |        1
   ;   17- 32 |       2 |        1
   ;   33- 48 |       3 |        1
   ; [...]
   ;  113-128 |       8 |        1
   ;  129-143 |       9 |        2
   ; [...]
   ;  241-256 |      16 |        2
   ;  257-272 |      17 |        3

   lda   tmp16+1
   lsr                  ; get 9th bit into carry
   lda   tmp16+0
   ror                  ; divide by 16 while using 9th bit
   lsr
   lsr
   lsr
   inc
   sta   cpm_nblck      ; number of block entries saved

   dec                  ; per 8 blocks 1 directory entry
   lsr
   lsr
   lsr
   inc
   sta   cpm_ndent      ; number of directory entries saved

   ; initialize table of free blocks
   ldx   #$00
   lda   #$ff
@loopinit:
   sta   freetab,x
   cpx   #$40
   bcs   :+
   sta   cpm_lblck,x    ; also clear out block list
:
   inx
   bne   @loopinit
   stz   freetab+$00    ; can't save in directory
   stz   freetab+$01
   stz   freetab+$fe    ; can't save in bootblocks
   stz   freetab+$ff    ; at end due to directory start offset

   ; scan directory for allocated blocks and remove them from free blocks
   ldx   #<i_usedblocks
   jsr   scandir

   ldx   #$00           ; index in blocklist (cpm_lblck)
@loopblocks:
   phx                  ; save blocklist index
:
   ; search free block
   ldx   cpm_cblck

   lda   freetab,x
   bne   @freefound
   inc   cpm_cblck
   bne   :-

   plx                  ; fix stack
   sec                  ; notify error: disk full
   rts

@freefound:
   ldy   #$ff
@findbit:
   iny
   lda   testmask,y
   and   freetab,x
   beq   @findbit       ; sector used, check next

   eor   freetab,x      ; mark sector as used by clearing that bit
   sta   freetab,x

   ; now convert to sector number
   ; X: bits 10-3, Y: bits 2-0 (not reversed)
   tya
   asl                  ; move bits to top for correct shifting later
   asl
   asl
   asl
   asl
   sta   tmp16+0

   txa
   stz   tmp16+1
   ldy   #$03
:
   asl   tmp16+0
   rol
   rol   tmp16+1
   dey
   bne   :-
   ; now A contains bits 7-0, tmp16+1 bits 10-8

   plx                  ; fetch blocklist index back from stack
   sta   cpm_lblck,x
   inx
   lda   tmp16+1
   sta   cpm_lblck,x
   inx

   txa
   lsr
   cmp   cpm_nblck      ; got enough free blocks?
   bcc   @loopblocks

   ; create dir entries
   stz   cpm_cdent
   ldx   #<i_create
   jsr   scandir
   clc
   rts

;-------------------------------------------------------------------------
; helpers
;-------------------------------------------------------------------------

; check if a filename matches the buffer cpm_fname
; in:
; X: start of filename in buffer ($00,$20,$40,$60)
; Y: $00
; used by load, delete
checkname:
   lda   buffer,x
   and   #$7f           ; strip off magic bits
   cmp   cpm_fname,y
   bne   @notfound
   inx
   iny
   cpy   #$0c
   bne   checkname
@notfound:
   rts                  ; Z=1 match, Z=0 no match

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
   lda   #<buffer
   sta   ID_MEM+0
   lda   #>buffer
   sta   ID_MEM+1

   stz   IDREAD,x       ; read (X=0) or write (X=1)
   stz   cpm_dirty      ; now the buffer has to be clean
   rts

dh_getlblks:
   jsr   checkname      ; skip if filename doesn't match
   bne   @done

   lda   buffer,x       ; Xl ($c)
   asl
   asl
   asl
   asl
   tay                  ; Y = offset in saved block list

   inx                  ; Bc ($d, ignored)
   inx                  ; Xh ($e, skipped)
   inx                  ; Rc ($f)
   lda   buffer,x
   adc   cpm_nsect+0
   sta   cpm_nsect+0
   bcc   :+
   inc   cpm_nsect+1
:
   inx
   lda   buffer,x
   sta   cpm_lblck,y
   iny
   tya
   and   #$0f           ; copy 16 bytes (aligned)
   bne   :-
@done:
   rts

dh_erase:
   jsr   checkname      ; skip if filename doesn't match
   bne   @done

   txa
   and   #$e0           ; go back to start of entry
   tax
   lda   #$e5
   sta   buffer,x       ; mark as deleted
   sta   cpm_dirty      ; needs to be written back
@done:
   rts

dh_usedblocks:
   beq   @empty         ; Z=1 empty directory entry

   txa
   ora   #$0f           ; dump to start of block table minus one
   tax

@loopblocks:
   inx

   ; convert block to bitmask
   lda   #$00
   ldy   #$03           ; divide by 2^3 = 8
:
   ror   buffer+1,x
   ror   buffer+0,x
   rol
   dey
   bne   :-

   ; state now:
   ; block+1 should be $00
   ; block+0 should contain index within freetab
   ; A contains number of blocks used in bits 2-0, reversed
   tay
   lda   setmask,y
   ldy   buffer+0,x
   and   freetab,y
   sta   freetab,y
   inx

   txa
   inc
   and   #$0f           ; 16 block bytes processed?
   bne   @loopblocks
@empty:
   rts

dh_create:
   lda   cpm_cdent
   cmp   cpm_ndent      ; all required entries created?
   bcs   @done          ; then nothing to do here

   lda   buffer,x
   cmp   #$e5           ; is it an empty entry?
   bne   @done          ; no, then nothing to do here

   sta   cpm_dirty      ; we're going to modify the sector now
:
   lda   cpm_fname,y    ; copy entry from cpm_fname to
   sta   buffer,x       ; St F0 F1 F2 F3 F4 F5 F6 F7 E0 E1 E2
   inx
   iny
   cpy   #$0c
   bcc   :-

   lda   cpm_cdent
   sta   buffer,x       ; X=$0c: Xl

   asl
   asl
   asl
   asl
   tay                  ; = offset in blocklist

   inx
   stz   buffer,x       ; X=$0d: Bc
   inx
   stz   buffer,x       ; X=$0e: Xh
   inx

   lda   cpm_nsect+1
   bne   @subtract
   lda   cpm_nsect+0
   cmp   #$81
   bcc   @store

@subtract:
   sec
   lda   cpm_nsect+0
   sbc   #$80
   sta   cpm_nsect+0
   bcs   :+
   dec   cpm_nsect+1
:
   lda   #$80
@store:
   sta   buffer,x       ; X=$0f: Rc
:
   inx                  ; start at X=$10
   lda   cpm_lblck,y    ; Y starts at cdent * $10
   sta   buffer,x
   iny
   tya
   and   #$0f
   bne   :-

   inc   cpm_cdent      ; entry done, set counter for next

@done:
   rts

dh_getdir:
   ; Y=$00 (required by all dirhandlers)
   ; X=$00,$20,$40,$60 (start of direntry)
   ; A=first byte of entry (type)
   ; Z=1 -> deleted entry
   lda   buffer,x
   cmp   cpm_fname
   bne   @done

   lda   tmp16+1
   beq   @print

@copy:
   lda   buffer,x
   sta   (tmp16),y
   inx
   iny
   cpy   #$10
   bcc   @copy

   lda   tmp16+0
   adc   #$0f           ; adc #$10, but C=1 (always)
   sta   tmp16+0
   bcc   @done
   inc   tmp16+1
@done:
   rts

   ; display on screen
@print:
.if 0
   lda   buffer+$0a,x   ; check if system file
   bmi   @done
.endif

   lda   buffer+$0c,x
   bne   @done          ; not first entry, skip

   jsr   @space
   lda   #'['
   jsr   CHROUT
@ploop:
   inx
   lda   buffer,x
   and   #$7f
   jsr   CHROUT
   iny
   cpy   #$08
   bne   @nodot
   lda   #'.'
   jsr   CHROUT
@nodot:
   cpy   #$0b
   bcc   @ploop

   lda   #']'
   jsr   CHROUT
@space:
   lda   #' '
   jmp   CHROUT

jmphandler:
   jmp   (cpm_hndlr)

dirhandlers:
i_getlblks = * - dirhandlers
   .word dh_getlblks
i_erase = * - dirhandlers
   .word dh_erase
i_usedblocks = * - dirhandlers
   .word dh_usedblocks
i_create = * - dirhandlers
   .word dh_create
i_getdir = * - dirhandlers
   .word dh_getdir

setmask:
   ; index bits reversed (bit 0-2)
   .byte $7f,$f7,$df,$fd,$bf,$fb,$ef,$fe ; bit7 = block0

testmask:
   ; index bits normal (bit 2-0)
   .byte $80,$40,$20,$10,$08,$04,$02,$01 ; bit7 = block0

.out "   ==================="
.out .sprintf( "   CP/M fs size: $%04x", * - cpmname )
.out "   ==================="
