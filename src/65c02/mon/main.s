
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
; [ ] reorganize zeropage memory usage
; [ ] BRK handler
; [ ] (s) cpmfs save
; [ ] (l) cpmfs load
; [ ] (h) find/hunt
; [ ] (c) compare
; [ ] (f) fill
; [ ] (t) transfer

.ifp02
; configuration for NMOS 6502 toolkit
.define LOADSAVE      0
.define BRKADJUST     1
.else
; configuration for CMOS native rom
.define LOADSAVE      0
.define BRKADJUST     0
.endif

; also in use (temporary only), provided by kernel (4 bytes)
;TMP16       := $04
;PSAVE       := $06
;ASAVE       := $07

; shadow registers for CPU (7 bytes)
R_PC        := $f0
R_A         := R_PC + 2
R_X         := R_PC + 3
R_Y         := R_PC + 4
R_SP        := R_PC + 5
R_P         := R_PC + 6

; used for internal functions
MODE        := R_PC + 7
ADDR0       := $f8      ; the last one entered, must be after MODE
ADDR1       := ADDR0+2  ; the one typically used
ADDR2       := ADDR0+4  ; the last one used (indicator for "clean" up/down)

FORMAT      := ADDR0+6  ;{addr/1}   ;temp for opcode decode
LENGTH      := ADDR0+7  ;{addr/1}   ;temp for opcode decode

TMP8        := $ef

; handling addresses (from input, traverse, etc)

INBUF       := $0200
INBUF_SIZE  := $4e      ; 78 characters to fit 80 char screen width

.export     start
.export     CHROUT
.export     PRINT
.export     getfirstnonspace
.export     prterr
.export     prtsp
.export     prt3sp
.export     prtxsp
.export     skipspace
.export     newedit

.export     INBUF
.exportzp   ASAVE
.exportzp   PSAVE
.exportzp   MODE
.exportzp   ADDR0
.exportzp   ADDR1
.exportzp   ADDR2
.exportzp   R_PC
.exportzp   R_A
.exportzp   R_X
.exportzp   R_Y
.exportzp   R_SP
.exportzp   R_P
.exportzp   TMP8
.exportzp   TMP16
.exportzp   FORMAT
.exportzp   LENGTH

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

; from papertape.s
.import     papertape

; from registers.s
.import     go
.import     regdump
.import     regedit
.import     regsave
.import     regupdown


.segment "CODE"

   jsr   regsave
   lda   #$f2
   sta   R_PC+0
   lda   #$ff
   sta   R_PC+1
   jmp   start

prterr:
   ;sta   $DF01
   lda   #$0a
   jsr   CHROUT
.ifp02
   txa
   pha
.else
   phx
.endif
   inx
   jsr   prtxsp
   lda   #'^'
   jsr   CHROUT
.ifp02
   pla
   tax
.else
   plx
.endif
   ; TODO: check if rts is okay, of if pla:pla:jmp will be better
   rts

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

start:
   jsr   PRINT
   .byte 10,"Sorbus Monitor",0
   jsr   regdump

newenter:
   lda   #$0a
   jsr   CHROUT
   bit   MODE
   bvc   :+
   lda   MODE
   and   #$bf           ; clear $40
   sta   MODE
   clv
   bne   newedit
:
.ifp02
   lda   #$00
   sta   INBUF
.else
   stz   INBUF
.endif
newedit:
   ldy   #VT100_CPOS_SOL
   int   VT100
   lda   #'.'           ; prompt char
   jsr   CHROUT

   lda   #<INBUF
   ldx   #>INBUF
   ldy   #INBUF_SIZE
   int   LINEINPUT

   beq   @enter
   cmp   #$03           ; CTRL+C
   beq   newenter
   cmp   #$c1
   beq   @updown
   cmp   #$c2
   bne   newedit
@updown:
   jsr   handleupdown   ; should return with X = size of INBUF
   jmp   newedit
@enter:
   jsr   handleenter
   jmp   newenter


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
   bmi   newenter

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
   .byte ":;>~ADGMRZ"
.if LOADSAVE
   .byte "LS"
.endif
@funcs:
   .word hexenter-1     ; :
   .word papertape-1    ; ;
   .word assembleedit-1 ; >
   .word regedit-1      ; ~
   .word assemble-1     ; A
   .word disassemble-1  ; D
   .word go-1           ; G
   .word memorydump-1   ; M
   .word regdump-1      ; R
   .word test-1         ; Z
.if LOADSAVE
   .word load-1         ; L
   .word save-1         ; S
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

assembleedit:
   stx   $DF01
   rts

test:
   lda   #<gotest
   sta   R_PC+0
   lda   #>gotest
   sta   R_PC+1
   rts

gotest:
   lda   #$0a
   jsr   CHROUT
   tsx
   inx                  ; correct JSR offset
   inx
   txa
   int   PRHEX8
   lda   #$0a
   jmp   CHROUT
