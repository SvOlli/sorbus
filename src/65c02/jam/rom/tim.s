;-------------------------------------------------------------------------
;  TIM for JAM core
;-------------------------------------------------------------------------

;-------------------------------------------------------------------------
; This code is mostly written and copyright by
; MOS Technology in 1976
;-------------------------------------------------------------------------
; Terminal Interface Monitor
; Code taken from 6530-004 RRIOT and heavily modified:
; - removed all bitbanging code to replace with subroutines for Sorbus
;     UART handling
; - processing of BRK, IRQ and NMI adapted to Sorbus vectors and handling
;     this also means that TIM in invoked using BRK #$00, instead of just
;     a single-byte BRK instruction (you can use "int $00", other
;     interrupts are handled by the kernel)
; - command interpreter changed to be more expandable and relocatable
; - adapted some of the code from 6502 to 65C02 assembler
;-------------------------------------------------------------------------

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

;  PROMPTING CHARACTER IS A PERIOD (.)
;  -----------------------------------


;  DISPLAY COMMANDS
;  ----------------

;  .R          DISPLAY REGISTERS (PC,F,A,X,Y,SP)
;  .M  ADDR    DISPLAY MEMORY ( 8 BYTES BEGINNING AT ADDR )


;  ALTER COMMAND (:)
;  -----------------
;  .:  DATA      ALTERS PREVIOUSLY DISPLAYED ITEM OR NEXT ITEM


;  PAPER TAPE I/O COMMANDS
;  ------------------------

;  .LH                   LOAD HEX TAPE
;  .WB ADDR1 ADDR2       WRITE BNPF TAPE (FROM LOW ADDR1 TO HIGH ADDR2)
;  .WH ADDR1 ADDR2       WRITE HEX TAPE (FROM LOW ADDR1 TO HIGH ADDR2)

;  CONTROL COMMANDS
;  ----------------

;  .G                    GO, CONTINUE EXECUTION FROM CIRRENT PC ADDRESS

;  .V                    TOGGLES VERIFY OPTION (DEFAULT: OFF)
;                          (IF ITS ON, TURNS IT OFF; IF OFF, TURNS ON)

;  BRK AND NMI ENTRY POINTS TO TIM
;  -------------------------------

;      TIM IS NORMALLY ENTERED WHEN A 'BRK' INSTRUKTION IS
;          ENCOUNTERED DURING PROGRAM EXECUTION.  AT THAT
;          TIME CPU REGISTERS ARE OUTPUT:    PC F A X Y SP
;          and   CONTROL   IS GIVEN TO THE KEYBOARD.
;      USER MAY ENTER TIM BY PROGRAMMED BRK OR INDUCED NMI.  NMI
;          ENTRIES CAUSE A '#' TO PRECEDE THE '.' IN THE CPU REGISTER
;          PRINTOUT FORMAT

;  NON-BRK INTRO (EXTERNAL DEVICE) INTERRUPT HANDLING
;  --------------------------------------------------

;      A NON-BRK INTRO INTERRUPT CAUSES AN INDIRECT JUMP TO THE ADDRESS
;          LOCATED AT 'UINT' (HEX FFF8).  THIS LOCATION CAN BE SET
;          USING THE ALTER CMD, OR LOADED AUTOMATICALLY IN PAPER TAPE
;          FROM WITH THE LH CMD IF THE USER ASSIGNS HIS INTRO INTERRUPT
;          VECTOR TO $FFF8 IN THE SOURCE ASSEMBLY PROGRAM.
;      IF NOT RESET BY THE USER, UINT IS SET TO CAUSE EXTERNAL
;          DEVICE INTERRUPTS TO ENTER TIM AS NMI'S.  I.E.,
;          IF A NMI OCOURS WITHOUT AN INDUCED NMI SIGNAL, IT IS
;          AN EXTERNAL DEVICE INTERRUPT.
;      Note: This has been changed for Sorbus from FFF8 to DF7C (UBRK)

;  SETTING AND RESETTING PROGRAM BREAKPOINTS
;  -----------------------------------------

;      BREAKPOINTS ARE SET AND RESET USING THE MEMORY DISPLAY
;          AND ALTER COMMANDS.  BRK HAS A '00' OPERATION CODE.
;      TO SET A BREAKPOINT SIMPLY DISPLAY THE MEMORY LOCATION
;          (FIRST INSTRUCTION.byte) AT WHICH THE BREAKPOINT IS
;          TO BE PLACED THEN ALTER THE LOCATION TO '00'.  THERE IS
;          NO LIMIT TO THE NUMBER OF BREAKPOINTS THAT CAN BE
;          ACTIVE AT ONE TIME.
;      TO RESET A BREAKPOINT, RESTORE THE ALTERED MEMORY LOCATION
;          TO ITS ORIGINAL VALUE.
;      WHEN AND IF A BREAKPOINT IS ENCOUNTERED DURING EXECUTION,
;          THE BREAKPOINT DATA PRECEDEC BY AN ':' IS DISPLAYED.
;          THE PROGRAM COUNTER VALUE DISPLAYED IS THE BRK
;          INSTRUCTION LOCATION + 1.

;  -------------------------------------------------------------------

; zeropage addresses used
WRAP      := $E8 ; 232              ;ADDRESS WRAP-AROUND FLAG
DIFF      := $E9 ; 233
VFLAG     := $EA ; 234
PREVC     := $EB ; 235
ACMD      := $EC ; 236
TMP0      := $EE ; 238
TMP2      := $F0 ; 240
TMP4      := $F2 ; 242
TMP6      := $F4 ; 244
PCL       := $F6 ; 246
PCH       := $F7 ; 247
FLGS      := $F8 ; 248
ACC       := $F9 ; 249
XR        := $FA ; 250
YR        := $FB ; 251
SP        := $FC ; 252
SAVX      := $FD ; 253
TMPC      := $FE ; 254
TMPC2     := $FF ; 255
RCNT      := TMPC
LCNT      := TMPC2

NCMDS     := <(ADRHIS-ADRLOS)

.include "jam.inc"
.include "jam_bios.inc"
.include "jam_kernel.inc"

.segment "CODE"

timstart:
   ldx   #$05
:
   lda   timvecs,x      ; initalize int vectors
   sta   UVBRK,x
   dex
   bpl   :-

   txs
   inx
   cli                  ; enable ints
   txa
   tay
   int   $00            ; TIM repurposes Sorbus user interrupt vector
   jmp   ($FFFC)        ; continue with reset

timvecs:
   .word timbrk         ; UVBRK
   .word timnmint       ; UVNMI
   .word timintrq       ; UVNBI

timnmint:
   sta   ACC
   lda   #'#'           ; set A=# to indicate nmint entry
   bne   bintcom        ; jmp to interrupt common code
timintrq:
   sta   ACC            ; save acc
   lda   #'%'           ; set A=% to indicate nmirq entry
   bne   bintcom        ; jmp to interrupt common code
timbrk:
   sta   ACC            ; save acc
   pla                  ; pull jsr return address saved on stack by kernel BRK routine
   pla                  ; pull jsr return address saved on stack by kernel BRK routine
   lda   #' '           ; space shows brk
bintcom:
   sta   TMPC           ; save int type flag
   cld                  ; clear decimal mode
   ;lsr                  ; # is odd, space is even
                        ; set cy for pc brk correction

   stx   XR             ; save X
   sty   YR             ; save Y
   pla
   sta   FLGS           ; save flags
   pla
   ;adc   #$FF           ; cy set to pc-1 for brk (Sorbus uses BRK as 2-byte opcode)
   sta   PCL
   pla
   ;adc   #$FF
   sta   PCH
   tsx
   stx   SP             ; save orig sp

   jsr   PRINT
   .byte $0d,$0a,"    ADDR P  A  X  Y  S",$0d,$0a,$00
   ;jsr   CRLF

   ldx   TMPC

   lda   #'*'
   jsr   WRTWO

   lda   #'R'           ; set for r display to permit
   bne   S0             ;   immediate alter following breakpoint.

start:
   lda   UARTCF
   and   #$fd           ; disable flow control
   sta   UARTCF
   lda   #$00           ; next command from user
   sta   WRAP           ; clear address wrap-around flag
   jsr   CRLF
   lda   #'.'           ; type prompting '.'
   jsr   CHROUT
   jsr   RDOC           ; read cmd, char returned in A

S0:
   ldx   #NCMDS-1       ; lock-up cmd
S1:
   cmp   CMDS,x
   bne   S2

   lda   SAVX           ; save previous cmd
   sta   PREVC
   stx   SAVX           ; save current cmd index
   lda   ADRHIS,x
   sta   ACMD+1         ;   all cmd code begins on mp1
   lda   ADRLOS,x
   sta   ACMD+0
   cpx   #$03           ; if :, r or m (0, 1, or 2) space 2
   bcs   IJMP
   jsr   spac2

IJMP:
   jmp   (ACMD)

S2:
   dex
   bpl   S1             ; loop for all cmds

ERROPR:
   lda   #'?'           ; operator err, type '?', restart
   jsr   CHROUT
.ifp02
   bpl   start          ; jmp   START (CHROUT returns n=0)
.else
   bra   start
.endif

DCMP:
   sec                  ; TMP2-TMP0 double subtract
   lda   TMP2+0
   sbc   TMP0+0
   sta   DIFF
   lda   TMP2+1
   sbc   TMP0+1
   tay                  ; return high order part in Y
   ora   DIFF           ; or lo for equ test
   rts

PUTP:
   lda   TMP0+0         ; move TMP0 to PCH,PCL
   sta   PCL
   lda   TMP0+1
   sta   PCH
   rts

ZTMP:
   lda   #$00           ; clear regs
   sta   TMP0+0,x
   sta   TMP0+1,x
   rts


;  read and store byte.  no store if space or rcnt=0.

BYTE:
   jsr   RDOB           ; char in A, cy=0 if sp
   bcc   BY3            ; space

   ldx   #$00           ; store byte
   sta   (TMP0,x)

   bit   VFLAG          ; test for verify flag
   bpl   BY2            ; skip if not set
   cmp   (TMP0,x)       ; test for valid write (RAM)
   beq   BY2
   pla                  ; err, clear jsr adr in stack
   pla
.ifp02
   jmp   ERROPR
.else
   bra   ERROPR
.endif

BY2:
   jsr   CADD           ; incr cksum
BY3:
   jsr   INCTMP         ; go incr tmpc adr
   dec   RCNT
   rts

SETR:
   lda   #FLGS          ; set to access regs
   sta   TMP0+0
.ifp02
   lda   #$00
   sta   TMP0+1
.else
   stz   TMP0+1
.endif
   lda   #$05
   rts

CMDS:     .byte ':'   , 'R'    , 'M'    , 'G', 'V' , 'L', 'W'
   ; W MUST BE LAST CMD IN CHAIN
ADRLOS:   .byte <ALTER, <DSPLYR, <DSPLYM, <GO, <VRFY, <LH, <WO
ADRHIS:   .byte >ALTER, >DSPLYR, >DSPLYM, >GO, >VRFY, >LH, >WO


;  display reg cmd - P,A,X,Y, AND SP

DSPLYR:
   jsr   WRPC           ; write pc
   jsr   SETR
   bne   M0             ; use DSPLYM

DSPLYM:
   jsr   RDOA           ; read mem adr into TMPC
   bcc   ERROPR         ; err if no addr
   lda   #$08           ; 8 bytes to display
M0:
   sta   TMPC
   ldy   #$00
M1:
   jsr   space          ; type 8 bytes of mem
   lda   (TMP0),y       ; (TMP0) preserved for poss alter
   jsr   WROB
   iny                  ; incr index
   dec   TMPC
   bne   M1
BEQS1:
   jmp   start

;  ALTER LAST DISPLAYED ITEM (ADR IN TMPC)

ALTER:
   dec   PREVC          ; r index = 1
   bne   A3

   jsr   RDOA           ; cy=0 if SP
   bcc   A2             ; space
   jsr   PUTP           ; alter pc
A2:
   jsr   SETR           ; alter r*s
   bne   A4             ; jmp a4 (setr returns acc = 5)
A3:
   jsr   WROA           ; alter M, type adr
   lda   #$08           ; set cnt=8

A4:
   sta   RCNT
A5:
   jsr   space          ; preserves Y
   jsr   BYTE
   bne   A5
   beq   BEQS1

GO:
   ldx   SP
   txs                  ; orig or new SP value to SP
   lda   PCH
   pha
   lda   PCL
   pha
   lda   FLGS
   pha
   lda   ACC
   ldx   XR
   ldy   YR
   rti

VRFY:
   lda   VFLAG
   eor   #$80
   sta   VFLAG
   asl
   rol
   ora   #'0'
   jsr   CHROUT
   jmp   start

LH:
   lda   UARTCF
   ora   #$02           ; enable flow control
   sta   UARTCF
   jsr   RDOC           ; read second cmd char -> and ignore, 'L' is always 'LH'
   jsr   CRLF
LH1:
   jsr   RDOC
   and   #$fe           ; SORBUS: addition because the output of WH is ';'
   cmp   #':'           ; find next bcd mark (:)
   bne   LH1            ; loop until found

   ldx   #$04
   jsr   ZTMP           ; clear cksum regs TMP4
   jsr   RDOB           ; read hex value -> length
   bne   LH2            ; data remain

   beq   BEQS1          ; finished

LH2:
   sta   RCNT           ; RCNT -> save counter
   jsr   CADD           ; bcd lngh to cksum ; add to chksum
   jsr   RDOB           ; sa ho to TMP0+1 (sa=startaddress)
   sta   TMP0+1
   jsr   CADD           ; add to cksum
   jsr   RDOB           ; sa lo to TMP0
   sta   TMP0+0
   jsr   CADD           ; add to cksum

LH3:
   jsr   BYTE           ; byte sub/r decrs rcnt on exit
   bne   LH3
   jsr   RDOA           ; cksum from hex bcd to TMP0
   lda   TMP4+0         ; TMP4 to TMP2 for DCMP
   sta   TMP2+0
   lda   TMP4+1
   sta   TMP2+1
   jsr   DCMP
   beq   LH1
   jmp   ERROPR

WO:
   jsr   RDOC           ; rd 2nd cmd char
   sta   TMPC
   jsr   space
   jsr   RDOA
   jsr   T2T2           ; sa to TMP2
   jsr   space          ; space before next address
   jsr   RDOA
   jsr   T2T2           ; sa to TMP0, ea to TMP2
   jsr   RDOC           ; delay for final CR
   lda   TMPC

   cmp   #'H'
   bne   WB

WH0:
   ldx   WRAP           ; if addr has wrapped around
   bne   BCCST          ; then terminate write operation

   jsr   CRLF
   ldx   #$18
   stx   RCNT           ; rcnt=24
   ldx   #$04           ; clear cksum
   jsr   ZTMP

   lda   #';'
   jsr   CHROUT         ; wr bcd mark

   jsr   DCMP           ; ea-sa (TMP0+2-TMP0) diff in LOC DIFF,+1
   tya                  ; ms byte of diff
   bne   WH1
   lda   DIFF
   cmp   #$17
   bcs   WH1            ; diff gt 24
   sta   RCNT           ; incr last RCNT
   inc   RCNT
WH1:
   lda   RCNT
   jsr   CADD           ; add to cksum
   jsr   WROB           ; bcd cnt in A
   lda   TMP0+1         ; sa ho
   jsr   CADD
   jsr   WROB
   lda   TMP0           ; sa lo
   jsr   CADD
   jsr   WROB

WH2:
   ldy   #$00
   lda   (TMP0),y

   jsr   CADD           ; inc CKSUM, preserves A
   jsr   WROB
   jsr   INCTMP         ; inc sa
   dec   RCNT
   bne   WH2            ; loop for op to 24 byte

   jsr   WROA4          ; write cksum

   jsr   DCMP
   bcs   WH0            ; loop while ea gt or = sa
BCCST:
   jmp   start


WB:
   inc   SAVX           ; SAVX to = NCMDS for ASCII sub/r
WB1:
   lda   WRAP           ; if addr has wrapped around
   bne   BCCST          ; then terminate write operation

   lda   #$04
   sta   ACMD
   jsr   CRLF
   jsr   WROA           ; output hex adr

WBNPF:
   jsr   space
   ldx   #9
   stx   TMPC           ; loop cnt = 9
   lda   (TMP0-9,x)
   sta   TMPC2          ; byte to TMPC2
   lda   #'B'
   bne   WBF2           ; write @

WBF1:
   lda   #'P'
   asl   TMPC2
   bcs   WBF2
   lda   #'N'

WBF2:
   jsr   CHROUT         ; write n or r
   dec   TMPC
   bne   WBF1           ; loop
   lda   #'F'
   jsr   CHROUT         ; write f

   jsr   INCTMP

   dec   ACMD           ; test for multiple of four
   bne   WBNPF

   jsr   DCMP
   bcs   WB1            ; loop while ea gt or = sa
   bcc   BCCST

CADD:
   pha                  ; save A
   clc
   adc   TMP4
   sta   TMP4
.if 1
   bcc   :+
   inc   TMP4+1
:
.else
   lda   TMP4+1
   adc   #$00
   sta   TMP4+1
.endif
   pla                  ; restore A
   rts

CRLF:
   ldx   #$0D
   lda   #$0A
   jmp   WRTWO

;  write adr from TMP0 stores

WROA:
   ldx   #$01
   bne   WROA1
WROA4:
   ldx   #$05
   bne   WROA1
WROA6:
   ldx   #$07
   bne   WROA1
WRPC:
   ldx   #$09
WROA1:
   lda   TMP0-1,x
   pha
   lda   TMP0,x
   jsr   WROB
   pla

;  write byte - A = byte
;  unpack byte data into two ASCII chars: A=byte; X,A=chars

WROB:
   pha
   lsr
   lsr
   lsr
   lsr
   jsr   ASCII          ; convert to ascii
   tax

   pla
   and   #$0F
   jsr   ASCII

;  write 2 chars - X,A = chars

WRTWO:
   pha
   txa
   jsr   CHROUT
   pla
   jmp   CHROUT

RDOC:
   jsr   chrinuc
   cmp   #$7f           ; del
   beq   RDOC           ; -> reject
.if 0
   cmp   #$0D           ; return
   beq   :+             ; -> accept
   cmp   #$20           ; other control character
   bcc   RDOC           ; -> reject
:
.endif
   jmp   CHROUT

ASCII:
   clc
   adc   #$06
   adc   #$F0
   bcc   ASC1
   adc   #$06

ASC1:
   adc   #$3A
   pha                  ; test for letter b in adr during wbnpf
   cmp   #'B'
   bne   ASCX
   lda   SAVX
   cmp   #NCMDS
   bne   ASCX           ; not wb cmd
   pla
   lda   #' '           ; for wb, blank @'s in adr
   pha
ASCX:
   pla
   rts

spac2:
   jsr   space
space:
   lda   #' '
   jmp   CHROUT

T2T2:
   ldx   #$02
T2T21:
   lda   TMP0-1,x
   pha
   lda   TMP2-1,x
   sta   TMP0-1,x
   pla
   sta   TMP2-1,x
   dex
   bne   T2T21
   rts

;increment (TMP0,TMP0+1) by 1
.ifp02
INCTMP:
   inc   TMP0           ; low byte
   beq   INCT1
   rts

INCT1:
   inc   TMP0+1         ; high byte
   beq   SETWRP
   rts

SETWRP:
   inc   WRAP           ; pointer has wrapped around - set flag
   rts
.else
   ; technically not 65C02 code, but this version uses 2 bytes less of ROM
INCTMP:
   inc   TMP0+0         ; low byte
   bne   INCT1
   inc   TMP0+1         ; high byte
   bne   INCT1
   inc   WRAP           ; pointer has wrapped around - set flag
   rts
INCT1:
   rts
.endif

; read hex adr; return ho in TMP0; LO in TMP0+1 and CY=1
;    if SP CY=0

RDOA:
   jsr   RDOB           ; read 2 char byte
   bcc   RDOA2          ; space

   sta   TMP0+1
RDOA2:
   jsr   RDOB
   bcc   RDEXIT         ; SP
   sta   TMP0
RDEXIT:
   rts

;  read hex byte and return in A, and cy=1
;    if SP CY=0
;    Y reg is preserved

RDOB:
.ifp02
   tya                  ; save Y
   pha
.else
   phy
.endif
   lda   #$00           ; set data = 0
   sta   ACMD
   jsr   RDOC
   cmp   #$0D           ; CR?
   bne   RDOB1
   pla                  ; yes - go to start
   pla                  ; cleaning stack up first
   pla
   jmp   start

RDOB1:
   cmp   #' '           ; space
   bne   RDOB2
   jsr   RDOC           ; read next char
   cmp   #' '
   bne   RDOB3
   clc                  ; CY=0
   bcc   RDOB4

RDOB2:
   jsr   hexit          ; to hex
   asl
   asl
   asl
   asl
   sta   ACMD
   jsr   RDOC           ; 2nd char assumed hex
RDOB3:
   jsr   hexit
   ora   ACMD
   sec                  ; CY=1
RDOB4:
.ifp02
   tax
   pla                  ; restore Y
   tay
   txa                  ; set Z & N flags for return
.else
   ply                  ; restore Y
   tax                  ; set Z & N flags for return
.endif
   rts

hexit:
   cmp   #$3A
   php                  ; save flags
   and   #$0F
   plp
   bcc   hex09          ; 0-9
   adc   #$08           ; alpha acc 8+CY=9
hex09:
   rts

.out "   ==============="
.out .sprintf( "   TIM size: $%04x", * - timstart )
.out "   ==============="
