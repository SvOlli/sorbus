
.P02
.include "../native.inc"
.include "../native_bios.inc"
.include "../native_kernel.inc"

.define INPUT_DEBUG 0

.import wozstart
.import Monitor
.import getkey

.export prhex16
.export prhex4
.export prhex8
.export prhex8s

ESC      := $1b
VT_LEFT  := 'D'
VT_RIGHT := 'C'
VT_SAVE  := 's'
VT_UNSAVE:= 'u'

.segment "CODE"
reset:
   jmp   start
   .byte "SBC23"

   jmp   inputtest
   jmp   getkeytest
;   jmp   Monitor

knownids:
   .byte $00,$01,$02,$06,$0E,$12,$21
cpunames:
   .byte "??",0,0
   .byte "02",0,0
   .byte "SC02"
   .byte "C02",0
   .byte "CE02"
   .byte "816",0
   .byte "65RA"

vectab:
   .word $0000          ; UVBRK: (unused) IRQ handler dispatches BRK
   .word uvnmi          ; UVNMI: hardware NMI handler
   .word $0000          ; UVNBI: (unused) IRQ handler dispatches non-BRK
   .word uvirq          ; UVIRQ: hardware IRQ handler

uvnmi:
uvirq:
   cld
   sta   TRAP
   rti

start:
   cld
   sei
   ldx   #$07           ; move to BIOS code?
:
   lda   vectab,x
   sta   UVBRK,x        ; setup user vectors
   dex
   bpl   :-

   txs                  ; initialize stack to $FF

   jsr   PRINT
   .byte 10,"NMOS 6502 toolkit",10,10,"CPU features: ",0

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
   .byte "BBSR ",0
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
   jsr   PRINT
   .byte "=> 65",0

   lda   CPUID
   ldx   #<(cpunames-knownids)
:
   dex
   beq   :+
   cmp   knownids,x
   bne   :-
:
   txa
   asl
   asl
   tax
   ldy   #$04
:
   lda   cpunames,x
   beq   :+
   jsr   CHROUT
   inx
   dey
   bne   :-
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

prhex8s:
   ; print hex byte in A without modifying A (required by WozMon2)
   pha
   jsr   prhex8
   pla
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

inputtest:
   jsr   PRINT
   .byte 10,"prompt> ",0
   lda   #$00
   sta   $0200
   ldx   #$02
   ldy   #$0f
   jsr   inputline
   jmp   $e000

getkeytest:
   jsr   getkey
   jsr   prhex8
   lda   #' '
   jsr   CHROUT
   jmp   getkeytest
