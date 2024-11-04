
.P02
.include "../native.inc"
.include "../native_bios.inc"
.include "../native_kernel.inc"

.import wozstart

.segment "CODE"
reset:
   jmp   start
   .byte "SBC23"

start:
   cld
   sei
   jsr   PRINT
   .byte 10,"CPU features: ",0
                   ; $01 NMOS, $02 CMOS, $04 BIT (RE)SET, $08 Z reg, $10 16 bit
   lda   CPUID
   lsr
   bcc   :+
   jsr   PRINT
   .byte "NMOS ",0
:
   lsr
   bcc   :+
   jsr   PRINT
   .byte "CMOS ",0
:
   lsr
   bcc   :+
   jsr   PRINT
   .byte "BBBSR ",0
:
   lsr
   bcc   :+
   jsr   PRINT
   .byte "Z-Reg ",0
:
   lsr
   bcc   :+
   jsr   PRINT
   .byte "16bit ",0
:

woz:
   lda   #$0a           ; start WozMon port
   jsr   CHROUT
:  ; workaround for WozMon not handling RTS when executing external code
   jsr   wozstart
   ; will be reached if own code run within WozMon exits using rts
   jmp   :-             ; no bra here: NMOS 6502 fallback mode

chrinuc:
   ; wait for character from UART and make it uppercase
   jsr   CHRIN
   bcs   chrinuc
uppercase:
   cmp   #'a'
   bcc   :+
   cmp   #'z'+1
   bcs   :+
   and   #$df
:
   rts

prhex16:
   ; output 16 bit value in X,A
   pha
   txa
   jsr   prhex8
   pla
   ; fall through
prhex8:
   ; output 8 bit value in A
   pha                  ; save A for LSD
   lsr                  ; move MSD down to LSD
   lsr
   lsr
   lsr
   jsr   prhex4         ; print MSD
   pla                  ; restore A for LSD
prhex4:
   and   #$0f           ; mask LSD for hex PRINT
   ora   #'0'           ; add ascii "0"
   cmp   #':'           ; is still decimal
   bcc   :+             ; yes -> output
   adc   #$06           ; adjust offset for letters A-F
:
   jmp   CHROUT

