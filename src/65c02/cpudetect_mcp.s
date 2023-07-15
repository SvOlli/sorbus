
.segment "CODE"
   ; should start at $0000
   ; whole memory is just $10 (=16) bytes
start:
   lda #$01
   .byte $eb,$ea ; 6502: sbc #$ea, 65C02: nop:nop, 65816: xba:nop
   sec
   lda   #$ea
   .byte $eb,$ea ; 6502: sbc #$ea, 65C02: nop:nop, 65816: xba:nop
   sta   $ff     ; 6502: A=$00,    65C02: A=$ea,   65816: A=$01
   jmp   start   ; this also includes the reset vector
   .byte $00,$ff ; irq vector used to store data

