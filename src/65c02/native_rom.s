; IMPORTANT NOTE
; ==============
;
; This is not the real rom for the native mode. It is just a minimal
; environment used to testing purposes.
;
; The read ROM code is in another castle. (ahm, sorry, repository)

.segment "CODE"

; zeropage addresses used by this part of ROM

ASAVE := $FC
PSAVE := $FD
TMP16 := $FE

.include "native_rom.inc"

; set to 65c02 code
; ...best not make use of opcode that are not supported by 65816 CPUs
.PC02

;-------------------------------------------------------------------------
; jumptable
;-------------------------------------------------------------------------

.if 0
; $E000: WozMon
   jmp   RESET
; $E003: run bootblock 0 from internal drive
   jmp   boot
; $E006: run bootblock id in $0f from internal drive
   jmp   bootx
; $E009: test internal drive (temporary)
   jmp   hddtest
; $E00C: copy $E000 RAM to $2000
   jmp   dumpram0
.endif

RESET:
   cld
   sei
   ldx   #$05
:
   lda   @vectab,x
   sta   UVNMI,x
   dex
   bpl   :-
   txs
@iloop:
   jsr   PRINT
   .byte 10,"Sorbus Native: 0-3)Boot, T)IM, W)ozMon? ",0
:
   jsr   chrin
   bcs   :-
   and   #$df     ; make uppercase
   cmp   #$10     ; '0'
   bcc   :+
   cmp   #$14     ; '4'
   bcc   @bootblock
:
   cmp   #'T'
   bne   :+
   jmp   timstart
:
   cmp   #'W'
   bne   @iloop
   lda   #$0a
   jsr   chrout
   jmp   wozstart
@bootblock:
   and   #$03
   jmp   boot+2

@vectab:
   .word todo
   .word todo
   .word todo

todo:
   stz   TRAP

bootx:
   lda   $0f
   .byte $2c   ; skip following 2-byte instruction
boot:
   lda   #$00
   ; boottrack is 32k, memory at $E000 is 8k
   ; starting at boot+2 can load other 8k offsets from internal drive
   jsr   PRINT ; preserves all registers... spooky.
   .byte 10,"Checking Bootsector... ",0
   ror
   ror
   ror
   and   #$c0
   sta   IDLBAL
   ldx   #$00
   stx   IDLBAH
   stx   IDMEML
   ldy   #$01        ; write sector to $0100 for checking
   sty   IDMEMH

   sta   IDREAD      ; jsr   secread

   lda   IDREAD
   beq   @checksig
   jsr   PRINT
   .byte "no internal drive",0
   bra   @errend

@notfound:
   jsr   PRINT
   .byte "no boot signature",0
@errend:
   jsr   PRINT
   .byte " found.",10,0
   jmp   $E000

@checksig:
   dec   IDLBAL    ; reset LBA for re-reading to $E000
   stx   IDMEML    ; adjust target memory to
   lda   #$E0      ; $E000 following
   sta   IDMEMH

   ldx   #$04
:
   lda   $0103,x
   cmp   @signature,x
   bne   @notfound
   dex
   bpl   :-
   
   jsr   PRINT
   .byte "found.",10,"Loading E000 from bootblock ",0
   lda   IDLBAL
   rol
   rol
   rol
   and   #$03
   ora   #$30
   jsr   chrout

   ldy   #$40        ; banksize ($2000) / sectorsize ($80)
:
   sta   IDREAD      ; jsr   secread
   dey
   bne   :-
   ; leave with Y = 0, as required in @jmpcode
   
   ldx   #$05
:
   lda   @jmpcode,x
   sta   $0180,x
   dex
   bpl   :-
;   jmp   wozmon
   jsr   PRINT
   .byte 10,"Jumping to E000 in bank 0",10,0
   jmp   $0180
   
@signature:
   .byte "SBC23"
@signatureend:

@jmpcode:
   stz   BANK      ; Y = 0
   jmp   $E000
   ;sta   TRAP
   inc   BANK     ; BANK has to be $00 upon call
   jsr   copybios
   stz   BANK
   rts
@jmpcodeend:

copybios:
   ldx   #$00
:
   lda   BIOS,x
   sta   BIOS,x
   inx
   bne   :-
   rts

.if 0
secread:
   jsr   PRINT
   .byte "loading sector ",0
   lda   IDLBAH
   jsr   prbyte
   lda   IDLBAL
   jsr   prbyte
   jsr   PRINT
   .byte " to ",0
   lda   IDMEMH
   jsr   prbyte
   lda   IDMEML
   jsr   prbyte
   sta   IDREAD
   lda   #$0a
   jmp   chrout
.endif

.if 0
dumpram0:
   ldy   #@dumpcodeend-@dumpcode
:
   lda   @dumpcode,y
   sta   $0280,y
   dey
   bpl   :-
   iny
   sty   $04
   lda   #$e0
   sta   $05
   sty   $06
   lda   #$20
   sta   $07
   jmp   $0280
@dumpcode:
   dec   BANK
:
   lda   ($04),y
   sta   ($06),y
   iny
   bne   :-
   inc   $07
   inc   $05
   bne   :-
   inc   BANK
   jmp   wozstart
@dumpcodeend:

hddtest:
   lda   #$00
   sta   IDLBAL
   sta   IDLBAH
   sta   IDMEML
   lda   #$20 ; write sectors to $2000 following
   sta   IDMEMH
   ldx   #$10 ; read first 16 sectors to $2000-$27FF
:
   sta   IDREAD
   dex
   bne   :-

   ; X is still 0
   stx   IDLBAL
   stx   IDMEML
   lda   #$20
   sta   IDLBAH
   sta   IDMEMH

   ldx   #$10
:
   sta   IDWRT
   dex
   bne   :-

   jsr   PRINT
   .byte 10,"SUCCESS",10,0

   jmp   $E000
.endif

bankjsr:
   php
   pha
   phx
   phy
   tsx
   lda   $0103,x     ; find address lobyte of payload address
   sta   TMP16+0     ; save it
   clc
   adc   #$02        ; skip two bytes on return address (payload)
   sta   $0103,x
   lda   $0104,x     ; find address hibyte of payload address
   sta   TMP16+1     ; save it
   bcc   :+
   inc   $0104,x     ; adjust hibyte of return address, if required
:
   ldy   #$01        ; jsr stores return address - 1 on stack, compensate
   lda   (TMP16),y   ; get lobyte of payload (target address)
   pha
   iny
   lda   (TMP16),y   ; get hibyte of payload (target address)
   sta   TMP16+1     ; reuse vector for jump address
   pla
   sta   TMP16+0
   ply
   plx
   pla
   plp
   jmp   (TMP16)

.segment "BIOS"
BIOS:
CHRIN:
   jmp   chrin
CHROUT:
   jmp   chrout
PRINT:
   jmp   print
BANKJSR:
   php
   pha
   lda   BANK
   sta   PSAVE
   lda   #$01
   sta   BANK
   pla
   plp
   jsr   bankjsr
   php
   pha
   lda   PSAVE
   sta   BANK
   pla
   plp
   rts

chrin:
   lda   UARTRS ; check input
   bne   :+     ; data available, fetch it
   sec          ; no input -> return 0 and set carry
   rts
:
   lda   UARTRD ; get key value
   clc
   rts

chrout:
   bit   UARTWS
   bmi   chrout
   sta   UARTWR
   rts

print:
   sta   ASAVE
   php
   pla
   sta   PSAVE
   pla
   sta   TMP16+0
   pla
   sta   TMP16+1
@loop:
   inc   TMP16+0
   bne   :+
   inc   TMP16+1
:
   lda   (TMP16)
   beq   @out
   jsr   chrout
   bra   @loop
@out:
   lda   TMP16+1
   pha
   lda   TMP16+0
   pha
   lda   PSAVE
   pha
   lda   ASAVE
   plp
   rts

NMI:
   jmp   (UVNMI)
IRQ:
   jmp   (UVIRQ)

.segment "VECTORS"
   .word NMI
   .word RESET
   .word IRQ
