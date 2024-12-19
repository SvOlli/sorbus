      lda #$80
      sta $df04
      lda #$93
      sta $df04
loop: lda $df02     ; A=rnd
      sta $10       ; ZP(0)=A
      lda $df02
      and #$03      ; A=A&3
      ora #$cc      ; A+=2
      sta $11       ; ZP(1)=A
      lda $df02     ; A=rnd
      sta ($10)     ; ZP(0),ZP(1)=y
      inx
      bne :+
      iny
      bne :+
      ldy #$fd
      stz $df04
:
      jmp loop
