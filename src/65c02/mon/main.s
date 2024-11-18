
.include "../native_bios.inc"
.include "../native.inc"

; TODO:
; [X] (m) memory dump
; [X] (:) memory edit
; [X] (r) register dump
; [X] (~) register edit
; [X] (g) go
; [X] (;) papertape load
; [ ] (s) cpmfs save
; [ ] (l) cpmfs load
; [ ] (d) disassemble
; [ ] (a) assemble
; [ ] (h) find/hunt
; [ ] (c) compare
; [ ] (f) fill


LMNEM           :=     $e6    ;{addr/1}   ;temp for mnemonic decoding
RMNEM           :=     $e7    ;{addr/1}   ;temp for mnemonic decoding
FORMAT          :=     $e8    ;{addr/1}   ;temp for opcode decode
LENGTH          :=     $e9    ;{addr/1}   ;temp for opcode decode
PCL             :=     $ee    ;{addr/1}   ;temp for program counter
PCH             :=     $ef    ;{addr/1}

R_PC        := $f0
R_A         := R_PC + 2
R_X         := R_PC + 3
R_Y         := R_PC + 4
R_SP        := R_PC + 5
R_P         := R_PC + 6

MODE        := R_PC + 7
;ASAVE       := R_PC + 8

ADDR0       := $e0      ; the last one entered
ADDR1       := ADDR0+2  ; the one typically used
ADDR2       := ADDR0+4  ; the last one used (indicator for "clean" up/down)

TMP8        := $fd

TMPVEC      := $fe

INBUF       := $0200
INBUF_SIZE  := $4e      ; 78 characters to fit 80 char screen width

.export     start
.export     CHROUT
.export     PRINT
.export     getfirstnonspace
.export     prtsp
.export     skipspace

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

; from asciihex.s
.import     uppercase

; from disassembler.s
.import     disassemble

; from hexdump.s
.import     hexenter
.import     hexupdown
.import     memorydump
.import     prhex8

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

prtsp:
   lda   #' '
   jmp   CHROUT

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
.ifp02
   lda   #$00
   sta   INBUF,x
.else
   stz   INBUF,x
.endif
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
   sta   TMP8
   tya
   asl
   tay
   lda   @funcs+1,y
   pha
   lda   @funcs+0,y
   pha
   inx
   lda   TMP8
   rts

@cmds:
   .byte ":;>~ADGMRZ"
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
assemble:
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
