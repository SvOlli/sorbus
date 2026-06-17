
; This is a port of SWEET16 as describe by Steve Wozniak in
; "SWEET16: The 6502 Dream Machine"
; Byte Magazine Volume 02 Number 11 (1977), pages 151-159

; ***********************
; *                     *
; *   APPLE-II PSEDUO   *
; * MACHINE INTERPRETER *
; *                     *
; *     S. WOZNIAK      *
; * APPLE COMPUTER INC  *
; *                     *
; ***********************

; adapted to the Sorbus Computer by "SvOlli" Sven Oliver Moll


.include "jam.inc"
.include "jam_bios.inc"

.export  sweet16

.segment "CODE"

sweet16regs := $e0
r0l         := sweet16regs + $00
r0h         := sweet16regs + $01
r14h        := sweet16regs + $1d
r15l        := sweet16regs + $1e
r15h        := sweet16regs + $1f

.ifpsc02
.else
.warning "expected to be configured as 65sc02"
.endif

; this must be at beginning of page
; after a jmp at the $E000 is also okay
; bottom of file contains sanity check

.segment "ROMSTART"

ld:
   lda   r0l,x
   sta   r0l
   lda   r0h,x          ;move rx to r0
   sta   r0h
   rts

st:
   lda   r0l
   sta   r0l,x          ;move r0 to rx
   lda   r0h
   sta   r0h,x
   rts

stat:
   lda   r0l
stat2:
   sta   (r0l,x)        ;store byte indirect
.ifpsc02
stat3:
   stz   r14h           ;indicate r0 is result neg
.else
   ldy   #$00
stat3:
   sty   r14h           ;indicate r0 is result neg
.endif
inr:
   inc   r0l,x
   bne   :+             ;incr rx
   inc   r0h,x
:
   rts

ldat:
   lda   (r0l,x)        ;load indirect (rx)
   sta   r0l            ;to r0
.ifpsc02
   stz   r0h            ;zero high order r0 byte
   bra   stat3
.else
   ldy   #$00
   sty   r0h            ;zero high order r0 byte
   beq   stat3          ;always taken
.endif

pop:
   ldy   #$00           ;high order byte = 0
.ifpsc02
   bra   pop2
.else
   beq   pop2           ;always taken
.endif
popd:                   ; always called with Y=$18 (r12)
   jsr   dcr            ;decr rx
   lda   (r0l,x)        ;pop high order byte @rx
   tay                  ;save in y reg
pop2:
   jsr   dcr            ;decr rx
   lda   (r0l,x)        ;low order byte
   sta   r0l            ;to r0
   sty   r0h
pop3:
.ifpsc02
   stz   r14h
.else
   ldy   #$00           ;indicate r0 as last result reg
   sty   r14h
.endif
   rts

lddat:
   jsr   ldat           ;low order byte to r0, incr rx
   lda   (r0l,x)        ;high order byte to r0
   sta   r0h
.ifpsc02
   bra   inr            ;incr rx
.else
   jmp   inr            ;incr rx
.endif

stdat:
   jsr   stat           ;store indirect low order
   lda   r0h            ;byte and incr rx. then
   sta   (r0l,x)        ;store high order byte.
.ifpsc02
   bra   inr            ;incr rx and return
.else
   jmp   inr            ;incr rx and return
.endif

stpat:
   jsr   dcr            ;decr rx
   lda   r0l
   sta   (r0l,x)        ;store r0 low byte @rx
.ifpsc02
   bra   pop3           ;indicate r0 as last result reg
.else
   jmp   pop3           ;indicate r0 as last result reg
.endif

dcr:
   lda   r0l,x
   bne   :+             ;decr rx
   dec   r0h,x
:
   dec   r0l,x
   rts

sub:
   ldy   #$00           ;result to r0
cpr:
   sec                  ;note y reg = 13*2 for cpr
   lda   r0l
   sbc   r0l,x
   sta   r0l,y          ;r0-rx to ry
   lda   r0h
   sbc   r0h,x
sub2:
   sta   r0h,y
   tya                  ;last result reg*2
   adc   #$00           ;carry to lsb
   sta   r14h
   rts

add:
   lda   r0l
   adc   r0l,x
   sta   r0l            ;r0+rx to r0
   lda   r0h
   adc   r0h,x
   ldy   #$00           ;r0 for result
   beq   sub2           ;finish add
bs:
   lda   r15l           ;note x reg is 12*2!
   jsr   stat2          ;push low pc byte via r12
   lda   r15h
   jsr   stat2          ;push high order pc byte
br:
   clc
bnc:
   bcs   bnc2           ;no carry test
br1:
   lda   (r15l),y       ;displacement byte
   bpl   br2
   dey
br2:
   adc   r15l           ;add to pc
   sta   r15l
   tya
   adc   r15h
   sta   r15h
bnc2:
   rts

bc:
   bcs   br
   rts

bp:
   asl                  ;double result-reg index
   tax                  ;to x reg for indexing
   lda   r0h,x          ;test for plus
   bpl   br1            ;branch if so
   rts

bm:
   asl                  ;double result-reg index
   tax
   lda   r0h,x          ;test for minus
   bmi   br1
   rts

bz:
   asl                  ;double result-reg index
   tax
   lda   r0l,x          ;test for zero
   ora   r0h,x          ;(both bytes)
   beq   br1            ;branch if so
   rts

bnz:
   asl                  ;double result-reg index
   tax
   lda   r0l,x          ;test for non-zero
   ora   r0h,x          ;(both bytes)
   bne   br1            ;branch if so
   rts

bm1:
   asl                  ;double result-reg index
   tax
   lda   r0l,x          ;check both bytes
   and   r0h,x          ;for $ff (minus 1)
   eor   #$ff
   beq   br1            ;branch if so
   rts

bnm1:
   asl                  ;double result-reg index
   tax
   lda   r0l,x
   and   r0h,x          ;check both bytes for no $ff
   eor   #$ff
   bne   br1            ;branch if not minus 1
   rts

bk:
   lda   (r15l)
   sta   TRAP           ;brk to monitor
nul:
   rts

rs:
   ldx   #$18           ;12*2 for r12 as stack pointer
   jsr   dcr            ;decr stack pointer
   lda   (r0l,x)        ;pop high return address to pc
   sta   r15h
   jsr   dcr            ;same for low order byte
   lda   (r0l,x)
   sta   r15l
   rts

set:
   bra   setz

rtn:
   ; here is $E0F7 @ 65sc02 ($E100 is max allowed)
   ; rtn is called using jsr, but exits sweet16
   pla
   pla
   tsx

   ;jmp   (r15l)         ;return to 6502 code via pc
   ; instead setup stack for proper return
   lda   r15l
   sta   $0104,x
   lda   r15h
   sta   $0105,x
   rts

setz:
   lda   (r15l),y       ;high order byte of constant
   sta   r0h,x
   dey
   lda   (r15l),y       ;low order byte of constant
   sta   r0l,x
   tya                  ;y reg contains 1
   sec
   adc   r15l           ;add 2 to pc
   sta   r15l
   bcc   :+
   inc   r15h
:
   rts


.segment "CODE"

sweet16:
   ; typical situation: SP=$FA; retaddr+0 @ $01FE
   tsx
   lda   $0105,x
   sta   r15h
   lda   $0104,x
   sta   r15l

   jsr   runopcode1     ;first iteration does not need increment of r15
mainloop:
   jsr   runopcode      ;interpret and execute
   bra   mainloop       ;one sweet16 instr.

runopcode:
   inc   r15l
   bne   runopcode1     ;incr sweet16 pc for fetch
   inc   r15h
runopcode1:
   lda   #>set          ;common high byte for all routines
   pha                  ;push on stack for rts
   ldy   #$00           ; not obsolete with 65sc02!
.ifpsc02
   lda   (r15l)         ;fetch instr
.else
   lda   (r15l),y       ;fetch instr
.endif
   and   #$0f           ;mask reg specification
   asl                  ;double for two byte registers
   tax                  ;to x reg for indexing
   lsr
.ifpsc02
   eor   (r15l)         ;now have opcode
.else
   eor   (r15l),y       ;now have opcode
.endif
   beq   tobr           ;if zero then non-reg op
   stx   r14h           ;indicate "prior result reg"
   lsr
   lsr                  ;opcode*2 to lsb's
   lsr
   tay                  ;to y reg for indexing
   lda   optbl-2,y      ;low order adr byte
   pha                  ;onto stack
   rts                  ;goto reg-op routine
tobr:
   inc   r15l
   bne   :+             ;incr pc
   inc   r15h
:
   lda   brtbl,x        ;low order adr byte
   pha                  ;onto stack for non-reg op
   lda   r14h           ;"prior result reg" index
   lsr                  ;prepare carry for bc, bnc.
   rts                  ;goto non-reg op routine

.segment "DATA"
brtbl:
   .byte <(rtn-1)         ;$00
optbl:
   .byte <(set-1)         ;$1x
   .byte <(br-1)          ;$01
   .byte <(ld-1)          ;$2x
   .byte <(bnc-1)         ;$02
   .byte <(st-1)          ;$3x
   .byte <(bc-1)          ;$03
   .byte <(ldat-1)        ;$4x
   .byte <(bp-1)          ;$04
   .byte <(stat-1)        ;$5x
   .byte <(bm-1)          ;$05
   .byte <(lddat-1)       ;$6x
   .byte <(bz-1)          ;$06
   .byte <(stdat-1)       ;$7x
   .byte <(bnz-1)         ;$07
   .byte <(pop-1)         ;$8x
   .byte <(bm1-1)         ;$08
   .byte <(stpat-1)       ;$9x
   .byte <(bnm1-1)        ;$09
   .byte <(add-1)         ;$Ax
   .byte <(bk-1)          ;$0A
   .byte <(sub-1)         ;$Bx
   .byte <(rs-1)          ;$0B
   .byte <(popd-1)        ;$Cx
   .byte <(bs-1)          ;$0C
   .byte <(cpr-1)         ;$Dx
   .byte <(nul-1)         ;$0D
   .byte <(inr-1)         ;$Ex
   .byte <(nul-1)         ;$0E
   .byte <(dcr-1)         ;$Fx
   .byte <(nul-1)         ;$0F

.assert >ld = >rtn, error, "sweet16 opcodes must not cross page"
