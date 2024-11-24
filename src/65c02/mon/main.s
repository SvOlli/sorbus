
.include "../native_bios.inc"
.include "../native.inc"

; TODO:
; [X] (m) memory dump
; [X] (:) memory edit
; [X] (r) register dump
; [X] (~) register edit
; [X] (g) go
; [X] (;) papertape load
; [X] (d) disassemble
; [X] (a) assemble
; [X] reorganize zeropage memory usage
; [X] BRK handler
; [X] native ROM integration
; [X] NMOS 6502 toolkit integration
; [ ] merge code?
; --- release build
; [ ] read respects bank register
; [X] (s) cpmfs save
; [X] (l) cpmfs load
; [ ] use .define FEATURE(s) instead of .ifp02
; --- sugarcoating starts here
; [ ] (h) find/hunt
; [ ] (c) compare
; [ ] (f) fill
; [ ] (t) transfer

.define PROMPT       '>'
.ifp02
.define LOADSAVE     0
.else
.define LOADSAVE     1
.endif
; TODO: replace in code
.define HEXPREFIX    ':'
.define PPTPREFIX    ';'
.define ASSPREFIX    '>'
.define REGPREFIX    '~'

; also in use (temporary only), provided by kernel (4 bytes)
;TMP16       := $04
;PSAVE       := $06
;ASAVE       := $07

; memory addresses used
;          a2h ass dis hex ppt reg
; $04 T16L
; $05 T16H
; $06 PSAV
; $07 ASAV
; $f6 TMP8
; $f7 MOD
; $f8 A0L
; $f9 A0H
; $fa A1L
; $fb A1H
; $fc A2L
; $fd A2H
; $fe FOR
; $ff LEN

; BRK_SA
; BRK_SX
; BRK_SY

; R_PC+0
; R_PC+1
; R_BK
; R_A
; R_X
; R_Y
; R_SP
; R_P

; used for internal functions
TMP8        := $f6
MODE        := $f7
ADDR0       := $f8      ; the last one entered, must be after MODE
ADDR1       := ADDR0+2  ; the one typically used
ADDR2       := ADDR0+4  ; the last one used (indicator for "clean" up/down)

FORMAT      := ADDR0+6  ;{addr/1}   ;temp for opcode decode
LENGTH      := ADDR0+7  ;{addr/1}   ;temp for opcode decode

; handling addresses (from input, traverse, etc)

INBUF       := $0200
INBUF_SIZE  := $4e      ; 78 characters to fit 80 char screen width

.export     getfirstnonspace
.export     prterr
.export     prtsp
.export     prt3sp
.export     prtxsp
.export     skipspace
.export     newedit
.export     clrenter
.export     newenter

.export     INBUF
.exportzp   MODE
.exportzp   ADDR0
.exportzp   ADDR1
.exportzp   ADDR2
.exportzp   TMP8
.exportzp   FORMAT
.exportzp   LENGTH

; from kernel
.ifp02
.import     inputline
.import     vt100
.endif

; from asciihex.s
.import     uppercase

; from assembler.s
.import     assemble

; from disassembler.s
.import     disassemble

; from hexdump.s
.import     hexenter
.import     hexupdown
.import     memorydump
.import     prthex8

; from interndrive.s
.import     blockrw

; from papertape.s
.import     papertape

; from registers.s
.import     go
.import     regdump
.import     regedit
.import     regsave
.import     regupdown
.import     inthandler

.ifp02
.else
; from storages.s
.import     directory
.import     loadfile
.import     savefile
.endif

.segment "CODE"

.if 0
bankread:
   lda   BRK_SB
   sta   @restore+1
   lda   (TMP16),y
   pha
@restore:
   lda   #$00
   sta   BRK_SB
   pla
   rts
.endif

prterr:
   lda   #$0a
   jsr   CHROUT
   inx                  ; add one for the prompt
   jsr   prtxsp
   lda   #'^'
   jsr   CHROUT
   pla
   pla
   jmp   newenter

prtsp:
   lda   #' '
   jmp   CHROUT
prt3sp:
   ldx   #$03
prtxsp:
:
   jsr   prtsp
   dex
   bne   :-
   rts

getfirstnonspace:
   ldx   #$ff
; get next non-space char
:
   inx
skipspace:
   lda   INBUF,x
   cmp   #' '
   beq   :-
   rts

clrenter:
.ifp02
   lda   #$00
   sta   INBUF
.else
   stz   INBUF
.endif
newenter:
   lda   #$0a
   jsr   CHROUT
newedit:
   ldy   #VT100_CPOS_SOL
.ifp02
   jsr   vt100
.else
   int   VT100
.endif
   lda   #PROMPT
   jsr   CHROUT

   lda   #<INBUF
   ldx   #>INBUF
   ldy   #INBUF_SIZE
.ifp02
   jsr   inputline
.else
   int   LINEINPUT
.endif

   beq   @enter
   cmp   #$03           ; CTRL+C
   beq   clrenter
   cmp   #$c1
   beq   @updown
   cmp   #$c2
   bne   newedit
@updown:
   jsr   handleupdown   ; should return with X = size of INBUF
   jmp   newedit
@enter:
   jsr   handleenter
   jmp   clrenter


handleenter:
   jsr   getfirstnonspace
   jsr   uppercase
   ldy   #(@funcs-@cmds-1)
:
   cmp   @cmds,y
   beq   @cmdfound
   dey
   bpl   :-
   ; no command was found
   jmp   prterr

@cmdfound:
   ; since X is used as index in input buffer can't use it here
   ; so jmp (addr,x) is out of a question, let's go the classic way
   ; at least there's no split between NMOS and CMOS here
   sta   TMP8           ; can be switched to LENGTH or FORMAT
   tya
   asl
   tay
   lda   @funcs+1,y
   pha
   lda   @funcs+0,y
   pha
   inx
   lda   TMP8           ; can be switched to LENGTH or FORMAT
   rts

@cmds:
   .byte ":;~ABDGMR"
.if LOADSAVE
   .byte "$LS"
.endif
@funcs:
   .word hexenter-1     ; :
   .word papertape-1    ; ;
   .word regedit-1      ; ~
   .word assemble-1     ; A
   .word blockrw-1      ; B
   .word disassemble-1  ; D
   .word go-1           ; G
   .word memorydump-1   ; M
   .word regdump-1      ; R
.if LOADSAVE
   .word directory-1    ; $
   .word loadfile-1     ; L
   .word savefile-1     ; S
.endif

handleupdown:
   tay                  ; save up/down key in Y
   jsr   getfirstnonspace
   cmp   #$00
   bne   :+
   lda   MODE
:
   cmp   #':'
   bne   :+
   jmp   hexupdown
:
   cmp   #'~'
   bne   :+
   jmp   regupdown
:
   rts
