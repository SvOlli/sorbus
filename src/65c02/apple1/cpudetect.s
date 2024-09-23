
.segment "CODE"

VECTOR := $24 ; reuse XAML/XAMH from wozmon: this is set upon start

start:

.if 0
   lda   #$01
.if 1
   .byte $eb,$ea
.else
   ; NMOS 6502
   sbc   #$ea
   ; CMOS 65C02
   nop            ; unofficial
   nop            ; official
   ; 65(C)816
   xba
   nop
.endif

   sec
   lda   #$ea
.if 1
   .byte $eb,$ea
.else
   ; NMOS 6502
   sbc   #$ea
   ; CMOS 65C02
   nop            ; unofficial
   nop            ; official
   ; 65(C)816
   xba            ; set a to previously stored $01
   nop
.endif
   jsr   $ffdc
   jmp   $ff00

.else
   jsr   print_generic   
   jsr   print_detect
   jmp   ($fffc)

print_detect:
; prepare B for 65(C)816
   lda   #$01
.if 1
   .byte $eb,$ea
.else
   ; NMOS 6502
   sbc   #$ea
   ; CMOS 65C02
   nop            ; unofficial
   nop            ; official
   ; 65(C)816
   xba
   nop
.endif

   sec
   lda   #$ea
.if 1
   .byte $eb,$ea
.else
   ; NMOS 6502
   sbc   #$ea
   ; CMOS 65C02
   nop            ; unofficial
   nop            ; official
   ; 65(C)816
   xba            ; set a to previously stored $01
   nop
.endif
   beq   is6502
   bmi   is65c02
   ldy   #<(txt_816-start)
   .byte $2c
is65c02:
   ldy   #<(txt_c02-start)
   .byte $2c
is6502:
   ldy   #<(txt_02-start)
   .byte $2c
print_generic:
   ldy   #<(txt_cpuis-start)
print_loop:
   lda   (VECTOR),y
   beq   @done
   ora   #$80
   jsr   $ffef
   iny
   bne   print_loop
@done:
   rts

txt_cpuis:
   .byte $0d,"CPU IS 65",$00
txt_c02:
   .byte "C"
txt_02:
   .byte "02",$0d,$00
txt_816:
   .byte "816",$0d,$00

.if 0
   ldx   #$00
:
   lda   $00,x
   sta   $df00,x
   inx
   bne   :-
   jmp   $ff00
.endif
.endif
