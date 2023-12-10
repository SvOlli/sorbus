;-------------------------------------------------------------------------
;  TIM for native core
;-------------------------------------------------------------------------

; Terminal Interface Monitor
; Code take from 6530-004 RRIOT and heavily modified:
; - removed all bitbanging code to replace with subroutines for UART handling
; - commands changed to be more expandable

; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

;  PROMPTING CHARACTER IS A PERIOD (.)
;  -----------------------------------


;  DISPLAY COMMANDS
;  ----------------

;  .R          DISPLAY REGISTERS (PC,F,A,x,y,SP)
;  .M  ADDR    DISPLAY MEMORY ( 8.byteS BEGINNING AT ADDR )


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

;  .H                    TOGGLES HIGH-SPEED-READER OPTION
;                          (IF ITS ON, TURNS IT OFF; IF OFF, TURNS ON)

;  BRK AND NMI ENTRY POINTS TO TIM
;  -------------------------------

;      TIM IS NORMALLY ENTERED WHEN A 'BRK' INSTRUKTION IS
;          ENCOUNTERED DURING PROGRAM EXECUTION.  AT THAT
;          TIME CPU REGISTERS ARE OUTPUT:    PC F A X Y SP
;          and   CONTrol   IS GIVEN TO THE KEYBOARD.
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
;          TO BE plaCED THEN ALTER THE LOCATION TO '00'.  THERE IS
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
CRDLY     := $E3 ; 227              ;DELAY FOR CR IN BIT-TIMES
WRAP      := $E4 ; 228              ;ADDRESS WRAP-AROUND FLAG
DIFF      := $E5 ; 229
HSPTR     := $E7 ; 231
HSROP     := $E8 ; 232
PREVC     := $E9 ; 233
MAJORT    := $EA ; 234
MINORT    := $EB ; 235
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

.include "native_rom.inc"

.segment "CODE"

   .byte "TIMSTART"

timstart:
timreset:
   ldx   #$05
:
   lda   timvecs,x      ; INITALIZE INT VECTORS
   sta   UVNMI,x
   dex
   bpl   :-

   txs
   inx
   stx   MAJORT         ; INIT MAJOR T COUNT To ZERO
   stx   HSPTR          ; CLEAR HSPTR FLAGS
   stx   HSROP
   cli                  ; ENABLE INTS
   BRK                  ; ENTER TIM BY BRK -> timintrq
   
timnmint:
   sta   ACC
   lda   #'#'          ; SET A=# TO INDICATE NMINT ENTRY
   bne   bintcom       ; jmp to interrupt common code
timintrq:
   sta   ACC           ; SAVE ACC
   pla               ; FLAGS TO A
   pha               ; RESTORE STACK STATUS
   and   #$10          ; TEST BRK FLAG
   beq   buirq         ; USER INTERRUPT

   asl               ; SET A=space (10 X 2 = 20)
bintcom:  sta   TMPC          ; SAVE INT TYPE FLAG
   cld               ; CLEAR DECIMAL MODE
   lsr               ; # IS ODD, space IS EVEN
                     ; SET CY FOR PC BRK CORRECTION

   stx   XR            ; SAVE X
   sty   YR            ; Y
   pla
   sta   FLGS          ; FLAGS
   pla
   adc   #$FF          ; CY SET TO PC-1 FOR BRK
   sta   PCL
   pla
   adc   #$FF
   sta   PCH
   tsx
   stx   SP            ; SAVE ORIG SP

   jsr   CRLF
   ldx   TMPC

   lda   #'*'
   jsr   WRTWO
   lda   #'R'          ; SET FOR R DISPLAY TO PERMIT
   bne   S0            ;   IMMEDIATE ALTER FOLLOWING BREAKPOINT.

buirq:
   lda   ACC
   jmp   (UVBRK)       ; control to user intrq service routine

start:
   lda   #$00          ;NEXT COMMAND FROM USER
   sta   HSPTR         ;CLEAR H. S. PAPER TAPE FLAG
   sta   WRAP          ;CLEAR ADDRESS WRAP-AROUND FLAG
   jsr   CRLF
   lda   #'.'          ; TYPE PROMPTING '.'
   jsr   WROC
   jsr   RDOC          ; READ CMD, CHAR RETURNED In A

   cmp   #'a'           ; check if convert to uppercase required
   bcc   S0
   cmp   #'z'           ;
   bcs   S0
   and   #$df           ; strip off lowercase bit
S0:
   ldx   #NCMDS-1       ; LOCK-UP CMD
S1:
   cmp   CMDS,x
   bne   S2

   lda   SAVX           ; SAVE PRIVIOUS CMD
   sta   PREVC
   stx   SAVX           ; SAVE CURRENT CMD INdex
   lda   ADRHIS,x
   sta   ACMD+1         ;   ALL CMD CODE BEGINS ON MP1
   lda   ADRLOS,x
   sta   ACMD+0
   cpx   #$03           ; IF :, R OR M (0, 1, OR 2) space 2
   bcs   IJMP
   jsr   spac2

IJMP:
   jmp   (ACMD)

S2:
   dex
   bpl   S1             ; LOOP FOR ALL CMDS

ERROPR:
   lda   #'?'           ; OPERATOR ERR, TYPE '?', RESTART
   jsr   WROC
   bcc   start          ; jmp   START (WROC RETURNS CY=0)

DCMP:
   sec                  ; TMP2-TMP0 DOUBLE SUBTRACT
   lda   TMP2+0
   sbc   TMP0+0
   sta   DIFF
   lda   TMP2+1
   sbc   TMP0+1
   tay                  ; RETURN HIGH ORDER PART IN Y
   ora   DIFF           ; OR LO FOR EQU TEST
   rts

PUTP:
   lda   TMP0+0         ; MOVE TMP0 TO PCH,PCL
   sta   PCL
   lda   TMP0+1
   sta   PCH
   rts

ZTMP:
   lda   #$00           ; CLEAR REGS
   sta   TMP0+0,x
   sta   TMP0+1,x
   rts


;  READ AND STOREBYTE.  NO STORE IF space OR RCNT=0.

BYTE:
   jsr   RDOB           ; CHAR IN A, CY=0 IF SP
   bcc   BY3            ; space

   ldx   #$00           ; STORE.byte
   sta   (TMP0,x)

   cmp   (TMP0,x)       ; TEST FOR VALID WRITE (RAM)
   beq   BY2
   pla                  ; ERR, CLEAR jsr   ADR IN STACK
   pla
   jmp   ERROPR

BY2:
   jsr   CADD           ; INCR CKSUM
BY3:
   jsr   INCTMP         ; GO INCR TMPC ADR
   dec   RCNT
   rts

SETR:
   lda   #FLGS          ; SET TO ACCESS REGS
   sta   TMP0
   lda   #$00
   sta   TMP0+1
   lda   #$05
   rts

CMDS:     .byte ':'   , 'R'    , 'M'    , 'G', 'H' , 'L', 'W' ; W MUST BE LAST CMD IN CHAIN
ADRLOS:   .byte <ALTER, <DSPLYR, <DSPLYM, <GO, <HSP, <LH, <WO
ADRHIS:   .byte >ALTER, >DSPLYR, >DSPLYM, >GO, >HSP, >LH, >WO


;  DISPLAY REG CMD - A,P,x,y, AND SP

DSPLYR:
   jsr   WRPC           ; WRITE PC
   jsr   SETR
   bne   M0             ; USE DSPLYM

DSPLYM:
   jsr   RDOA           ; READ MEM ADR INTO TMPC
   bcc   ERRS1          ; ERR IF NO ADDR
   lda   #$08
M0:
   sta   TMPC
   ldy   #$00
M1:
   jsr   space          ; TYPE 8.byteS OF MEM
   lda   (TMP0),y       ; (TMP0) PRESERVED FOR POSS ALTER
   jsr   WROB
   iny                  ; INCR INDEX
   dec   TMPC
   bne   M1
BEQS1:
   jmp   start

ERRS1:
   jmp   ERROPR

;  ALTER LAST DISPLAYED ITEM (ADR IN TMPC)

ALTER:
   dec   PREVC          ; R INdex = 1
   bne   A3

   jsr   RDOA           ; CY=0 IF SP
   bcc   A2             ; space
   jsr   PUTP           ; ALTER PC
A2:
   jsr   SETR           ; ALTER R*S
   bne   A4             ; jmp   A4 (SETR RETURNS ACC = 5)
A3:
   jsr   WROA           ; ALTER M, TYPE ADR
   lda   #$08           ; SET CNT=8

A4:
   sta   RCNT
A5:
   jsr   space          ; PRESERVES Y
   jsr   BYTE
   bne   A5
A9:
   beq   BEQS1

GO:
   ldx   SP
   txs                  ; ORIG OR NEW SP VALUE TO SP
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

HSP:
   inc   HSROP         ; TOGGLE BIT C
   jmp   start

LH:
   jsr   RDOC          ; READ secOND CMD CHAR
   jsr   CRLF
   ldx   HSROP         ; ENABLE PTR OPTION IF SET
   stx   HSPTR
LH1:
   jsr   RDOC
   cmp   #':'          ; FIND NEXT BCD MARK (:)
   bne   LH1

   ldx   #$04
   jsr   ZTMP          ; CLEAR CKSUM REGS TMP4
   jsr   RDOB
   bne   LH2

   ldx   #$00          ; CLEAR HS ror   FLAG
   stx   HSPTR
   beq   BEQS1         ; FINISHED

LH2:
   sta   RCNT          ; RCNT
   jsr   CADD          ; BCD LNGH TO CKSUM
   jsr   RDOB          ; SA HO TO TMP0+1
   sta   TMP0+1
   jsr   CADD          ; ADD TO CKSUM
   jsr   RDOB          ; SA LO TO TMP0
   sta   TMP0
   jsr   CADD          ; ADD TO CKSUM

LH3:
   jsr   BYTE          ; BYTE SUB/R DECRS RCNT On EXIT
   bne   LH3
   jsr   RDOA          ; CKSUM FROM HEX BCD TO TMP0
   lda   TMP4+0        ; TMP4 TO TMP2 FOR DCMP
   sta   TMP2+0
   lda   TMP4+1
   sta   TMP2+1
   jsr   DCMP
   beq   LH1
ERRP1:
   jmp   ERROPR

WO:
   jsr   RDOC           ; RD 2ND CMD CHAR
   sta   TMPC
   jsr   space
   jsr   RDOA
   jsr   T2T2           ; SA TO TMP2
   jsr   space          ; SPACE BEFORE NEXT ADDRESS
   jsr   RDOA
   jsr   T2T2           ; SA TO TMP0, EA TO TMP2
   jsr   RDOC           ; DELAY FOR FINAL CR
   lda   TMPC

   cmp   #'H'
   bne   WB

WH0:
   ldx   WRAP           ; IF ADDR HAS WRAPPED AROUND
   bne   BCCST          ; THEN TERMINATE WRITE OPERATION

   jsr   CRLF
   ldx   #$18
   stx   RCNT           ; RCNT=24
   ldx   #$04           ; CLEAR CKSUM
   jsr   ZTMP

   lda   #';'
   jsr   WROC           ; WR BCD MARK

   jsr   DCMP           ; EA-SA (TMP0+2-TMP0) DIFF IN LOC DIFF,+1
   tya                  ; MS.byte OF DIFF
   bne   WH1
   lda   DIFF
   cmp   #$17
   bcs   WH1            ; DIFF GT 24
   sta   RCNT           ; INCR LAST RCNT
   inc   RCNT
WH1:
   lda   RCNT
   jsr   CADD           ; ADD TO CKSUM
   jsr   WROB           ; BCD CNT IN A
   lda   TMP0+1         ; SA HO
   jsr   CADD
   jsr   WROB
   lda   TMP0           ; SA LO
   jsr   CADD
   jsr   WROB

WH2:
   ldy   #$00
   lda   (TMP0),y

   jsr   CADD           ; inc   CKSUM, PRESERVES A
   jsr   WROB
   jsr   INCTMP         ; inc   SA
   dec   RCNT
   bne   WH2            ; LOOP FOR OP TO 24.byte

   jsr   WROA4          ; WRITE CKSUM

   jsr   DCMP
   bcs   WH0            ; LOOP WHILE EA GT OR = SA
BCCST:
   jmp   start


WB:
   inc   SAVX           ; SAVX TO = NCMDS FOR ASCII SUB/R
WB1:
   lda   WRAP           ;IF ADDR HAS WRAPPED AROUND
   bne   BCCST          ;THEN TERMINATE WRITE OPERATION

   lda   #$04
   sta   ACMD
   jsr   CRLF
   jsr   WROA           ; OUTPUT HEX ADR

WBNPF:
   jsr   space
   ldx   #9
   stx   TMPC           ; LOOP CNT =9
   lda   (TMP0-9,x)
   sta   TMPC2          ; BYTE TO TMPC2
   lda   #'B'
   bne   WBF2           ; WRITE @

WBF1:
   lda   #'P'
   asl   TMPC2
   bcs   WBF2
   lda   #'N'

WBF2:
   jsr   WROC           ; WRIRE N OR R
   dec   TMPC
   bne   WBF1           ; LOOP
   lda   #'F'
   jsr   WROC           ; WRITE F

   jsr   INCTMP

   dec   ACMD           ; TEST FOR MULTIPLE OF FOUR
   bne   WBNPF

   jsr   DCMP
   bcs   WB1            ; LOOP WHILE EA GT OR = SA
   bcc   BCCST

CADD:
   pha                  ; SAVE A
   clc
   adc   TMP4
   sta   TMP4
   lda   TMP4+1
   adc   #$00
   sta   TMP4+1
   pla                  ; RESTORE A
   rts

CRLF:
   ldx   #$0D
   lda   #$0A
   jsr   WRTWO
   rts

;  WRITE ADR FROM TMP0 STORES

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

;  WRITE.byte - A = BYTE
;  UNPACK.byte DATA INTO TWO ASCII CHARS: A=BYTE; X,A=CHARS

WROB:
   pha
   lsr
   lsr
   lsr
   lsr
   jsr   ASCII         ; CONVERT TO ASCII
   tax
   pla
   and   #$0F
   jsr   ASCII

;  WRITE 2 CHARS - X,A = CHARS

WRTWO:
   pha
   txa
   jsr   WRT
   pla
WRT:
WROC:
   jmp   CHROUT
RDT:
RDOC:
   jsr   CHRIN
   bcs   RDOC
   jmp   CHROUT

ASCII:
   clc
   adc   #$06
   adc   #$F0
   bcc   ASC1
   adc   #$06

ASC1:
   adc   #$3A
   pha                  ; TEST FOR LETTER B IN ADR DURING WBNPF
   cmp   #'B'
   bne   ASCX
   lda   SAVX
   cmp   #NCMDS
   bne   ASCX           ; NOT WB CMD
   pla
   lda   #' '           ; FOR WB, BLANK @'S IN ADR
   pha
ASCX:
   pla
   rts

spac2:
   jsr   space
space:
   pha               ; SAVE A,x,y
   txa
   pha
   tya
   pha
   lda   #' '
   jsr   WRT           ; TYPE SP
   pla               ; RESTORE A,x,y
   tay
   pla
   tax
   pla
   rts

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

;INCREMENT (TMP0,TMP0+1) BY 1
INCTMP:
   inc   TMP0          ;LOW.byte
   beq   INCT1
   rts

INCT1:
   inc   TMP0+1        ;HIGH.byte
   beq   SETWRP
   rts

SETWRP:
   inc   WRAP          ;POINTER HAS WRAPPED AROUND - SET FLAG
   rts

; READ HEX ADR; RETURN HO IN TMP0; LO IN TMP0+1 AND CY=1
;    IF SP CY=0

RDOA:
   jsr   RDOB          ; READ 2 CHAR.byte
   bcc   RDOA2         ; space

   sta   TMP0+1
RDOA2:
   jsr   RDOB
   bcc   RDEXIT        ; SP
   sta   TMP0
RDEXIT:
   rts

;  READ HEX.byte AND RETURN IN A, AND CY=1
;    IF SP CY=0
;    Y REG IS PRESERVED

RDOB:
   tya                  ; SAVE Y
   pha
   lda   #$00           ; SET DATA = 0
   sta   ACMD
   jsr   RDOC
   cmp   #$0D           ; CR?
   bne   RDOB1
   pla                  ;YES - GO TO START
   pla                  ;CLEANING STACK UP FIRST
   pla
   jmp   start

RDOB1:
   cmp   #' '           ; space
   bne   RDOB2
   jsr   RDOC           ; READ NEXT CHAR
   cmp   #' '
   bne   RDOB3
   clc                  ; CY=0
   bcc   RDOB4

RDOB2:
   jsr   hexit          ; TO HEX
   asl
   asl
   asl
   asl
   sta   ACMD
   jsr   RDOC           ; 2ND CHAR ASSUMED HEX
RDOB3:
   jsr   hexit
   ora   ACMD
   sec                  ; CY=1
RDOB4:
   tax
   pla                  ; RESTORE Y
   tay
   txa                  ;SET Z & N FLAGS FOR RETURN
   rts

hexit:
   cmp   #$3A
   php               ; SAVE FLAGS
   and   #$0F
   plp
   bcc   hex09         ; 0-9
   adc   #$08          ; ALPHA ACC 8+CY=9
hex09:
   rts

timvecs:
   .word timnmint
   .word timnmint    ; default user intrq to nmint
   .word timintrq
