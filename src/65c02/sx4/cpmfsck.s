
.include "../native_bios.inc"
.include "../native.inc"

.define DEBUG 0

dirmem   := $1000
blockmap := $0800

tmpvec   := $10
dirpos   := $12
blockpos := $14

.assert (<dirmem)   = 0, error, "dirmem needs to be page aligned"
.assert (<blockmap) = 0, error, "blockmap needs to be page aligned"

start:
   jsr   PRINT
   .byte 10,"internal drive block map",10,0

   jsr   clearmap
.if DEBUG
   jsr   dumpmap
.endif

   ldx   #$00
   lda   #$01
   stx   ID_LBA+0
   sta   ID_LBA+1
   lda   #>dirmem
   stx   ID_MEM+0
   sta   ID_MEM+1
   stx   dirpos+0
   sta   dirpos+1

:
   sta   IDREAD
   inx
   bne   :-

@dirloop:
   ldy   #$00
   lda   (dirpos),y
   cmp   #$e5
   beq   @skipentry

.if DEBUG
   jsr   PRINT
   .byte 10,"entry:",0
   lda   dirpos+0
   ldx   dirpos+1
   int   PRHEX16
.endif
   
   ldy   #$10
@entryloop:
   iny
   lda   (dirpos),y
   tax                  ; hi-byte now in X
   dey
   ora   (dirpos),y
   beq   @skipentry     ; pointer empty -> branch out
   lda   (dirpos),y     ; lo-byte now in A
   iny
   iny

.if DEBUG
   jsr   PRINT
   .byte 10,"block:",0
   int   PRHEX16
.endif

   ; add offset (start count at directory)
   clc
   adc   #$10
   sta   blockpos+0
   ; add offset (blockmap start)
   txa
   adc   #>blockmap
   sta   blockpos+1
   lda   #'*'
   cmp   (blockpos)
   bne   :+
   lda   #'!'
:
   sta   (blockpos)

.if DEBUG
   jsr   PRINT
   .byte " -> ",0
   lda   blockpos+0
   ldx   blockpos+1
   int   PRHEX16
.endif

   cpy   #$20
   bcc   @entryloop

@skipentry:
   clc
   lda   dirpos+0
   adc   #$20
   sta   dirpos+0
   lda   dirpos+1
   adc   #$00
   sta   dirpos+1
   cmp   #$90
   bcc   @dirloop
   
   
   jsr   dumpmap
   jmp   ($fffc)
   

clearmap:
   ldy   #$00
   sty   tmpvec+0
   lda   #>blockmap
   sta   tmpvec+1
   tya
   ldx   #$08
   lda   #'.'
@loop:
   sta   (tmpvec),y
   iny
   bne   @loop
   inc   tmpvec+1
   dex
   bne   @loop
   ldy   #$0f
:
   lda   #'B'
   sta   blockmap+$00,y
   lda   #'D'
   sta   blockmap+$10,y
   dey
   bpl   :-
   rts

dumpmap:
   ldy   #$00
   sty   tmpvec+0
   lda   #>blockmap
   sta   tmpvec+1
   tya
   ldx   #$08
@loop:
   tya
   and   #$3f
   bne   :+
   lda   #$0a
   jsr   CHROUT
:
   lda   (tmpvec),y
   jsr   CHROUT
   iny
   bne   @loop
   inc   tmpvec+1
   dex
   bne   @loop
   rts
