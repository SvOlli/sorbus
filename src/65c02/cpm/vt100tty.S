; -------------------------------------------------------------------------
;
; 80 columns TTY/SCREEN driver for VT100 serial terminals
;
; Copyright © 2023 Sven Oliver Moll
;
; based upon:
;
; 80 columns TTY/SCREEN driver for the Atari 400/800 and XL/XE
;
; Copyright © 2023 Ivo van Poorten
; This file is licensed under the terms of the 2-clause BSD license. Please
; see the COPYING file in the root project directory for the full text.
;
; -------------------------------------------------------------------------

#include "zif.inc"

; force to include the clang version and not the asm.com version
#include "cpm65.inc"

#include "driver.inc"
#include "jumptables.inc"

ZEROPAGE

drv_zp_begin:

ptr:            .fill 2
ptr1:           .fill 2

drv_zp_end:

SCREEN_WIDTH  = 80
SCREEN_HEIGHT = 24
ESC           = 27

; -------------------------------------------------------------------------

zproc main
    jmp init
zendproc

; -------------------------------------------------------------------------

; SCREEN80 DESCRIPTION AND DISPATCHER

drv_vt100:
    .word DRVID_SCREEN
    .word drv_vt100_strat
    .word 0
    .ascii "SCREEN80"
    .byte 0

zproc drv_vt100_strat
    jmpdispatch drv_vt100_jump_lo, drv_vt100_jump_hi

drv_vt100_jump_lo:
    jmptablo vt100_version
    jmptablo vt100_getsize
    jmptablo vt100_clear
    jmptablo vt100_setcursor
    jmptablo vt100_getcursor
    jmptablo vt100_putchar
    jmptablo vt100_putstring
    jmptablo vt100_getchar
    jmptablo vt100_showcursor
    jmptablo vt100_scrollup
    jmptablo vt100_scrolldown
    jmptablo vt100_cleartoeol
    jmptablo vt100_setstyle

drv_vt100_jump_hi:
    ; #ROW & #COLUMN start with 1, not 0 !!!
    jmptabhi vt100_version
    jmptabhi vt100_getsize    ; keep to 80x24
    jmptabhi vt100_clear      ; ESC [ 2 J
    jmptabhi vt100_setcursor  ;!ESC [ <#ROW> ; <#COLUMN> H
    jmptabhi vt100_getcursor  ;!ESC [ 6 n  <-  ESC [ <#ROW> ; <#COLUMN> R
    jmptabhi vt100_putchar    ; BDOS_CONOUT
    jmptabhi vt100_putstring  ; BDOS_PRINTSTRING
    jmptabhi vt100_getchar    ; BDOS_CONIN
    jmptabhi vt100_showcursor ; -> RTS for now
    jmptabhi vt100_scrollup   ; ESC [ D
    jmptabhi vt100_scrolldown ; ESC [ M
    jmptabhi vt100_cleartoeol ; ESC [ 0 K
    jmptabhi vt100_setstyle   ; -> RTS for now
zendproc

zproc fail
    sec
    rts
zendproc

; -------------------------------------------------------------------------

; TTY80 DESCRIPTION AND DISPATCHER

drv_tty80:
    .word DRVID_TTY
    .word drv_tty80_strat
    .word 0
    .ascii "TTY80"
    .byte 0

zproc drv_tty80_strat
    jmpdispatch drv_tty80_jump_lo, drv_tty80_jump_hi

drv_tty80_jump_lo:
    jmptablo tty80_const
    jmptablo tty80_conin
    jmptablo tty80_conout

drv_tty80_jump_hi:
    jmptabhi tty80_const
    jmptabhi tty80_conin
    jmptabhi tty80_conout

zendproc

; -------------------------------------------------------------------------

; INITIALIZE DRIVERS

zproc init
    ldy #BDOS_GET_BIOS
    jsr BDOS
    sta BIOS+1
    stx BIOS+2

    lda #<msg1
    ldx #>msg1
    ldy #BDOS_WRITE_STRING
    jsr BDOS

; find current drivers which we reuse for keyboard input

    lda #<DRVID_SCREEN
    ldx #>DRVID_SCREEN
    ldy #BIOS_FINDDRV
    jsr BIOS
    sta next_screen+1
    stx next_screen+2

    lda #<msg2
    ldx #>msg2
    ldy #BDOS_WRITE_STRING
    jsr BDOS

    lda #<DRVID_TTY
    ldx #>DRVID_TTY
    ldy #BIOS_FINDDRV
    jsr BIOS
    sta next_tty+1
    stx next_tty+2

    lda #<msg3
    ldx #>msg3
    ldy #BDOS_WRITE_STRING
    jsr BDOS

; setup data

    ldx #$ff
    stx screen_width
    stx screen_height
    dex
    stx screen_width_1
    stx screen_height_1

    txa                 ; set width and height to maximum
    jsr vt100_setcursor

    lda #<msg4
    ldx #>msg4
    ldy #BDOS_WRITE_STRING
    jsr BDOS

    jsr vt100_getxy
    ; C=1 on error

    sta screen_width
    stx screen_height
    dex
    stx screen_height_1
    txa
    dex
    stx screen_width_1
    
    lda #<msg5
    ldx #>msg5
    ldy #BDOS_WRITE_STRING
    jsr BDOS

; disable BIOS cursor

    ;ldy #SCREEN_SHOWCURSOR
    ;lda #0
    ;jsr next_screen

; register new drivers

    lda #<drv_vt100
    ldx #>drv_vt100
    ldy #BIOS_ADDDRV
    jsr BIOS

    lda #<msg6
    ldx #>msg6
    ldy #BDOS_WRITE_STRING
    jsr BDOS

    lda #<drv_tty80
    ldx #>drv_tty80
    ldy #BIOS_ADDDRV
    jsr BIOS

    lda #<msg7
    ldx #>msg7
    ldy #BDOS_WRITE_STRING
    jsr BDOS

; claim memory

    ldy #BIOS_GETTPA
    jsr BIOS
    lda #>reserve_tpa_end
    ldy #BIOS_SETTPA
    jsr BIOS

    ldy #BIOS_GETZP
    jsr BIOS
    clc
    adc #drv_zp_end-drv_zp_begin
    ldy #BIOS_SETZP
    jsr BIOS

; do some hardware required stuff here... if required


; clear screen, home cursor, cache first curlineptr, print banner

    jsr vt100_clear

    lda #<banner
    ldx #>banner
    ldy #BDOS_WRITE_STRING
    jsr BDOS

    rts
zendproc

msg1:
   .ascii "1"
   .byte 10, 0
msg2:
   .ascii "2"
   .byte 10, 0
msg3:
   .ascii "3"
   .byte 10, 0
msg4:
   .ascii "4"
   .byte 10, 0
msg5:
   .ascii "5"
   .byte 10, 0
msg6:
   .ascii "6"
   .byte 10, 0
msg7:
   .ascii "7"
   .byte 10, 0

; -------------------------------------------------------------------------

; SCREEN80 IMPLEMENTATION

zproc vt100_version
    lda #0
    rts
zendproc

zproc vt100_getsize
    ;lda #SCREEN_WIDTH-1
    ;ldx #SCREEN_HEIGHT-1
    lda screen_width_1
    ldx screen_height_1
    rts
zendproc

zproc vt100_clear       ; ESC [ 2 J
    ldx #$01
    .byte $2c
    ; slip through
zendproc
zproc vt100_cleartoeol  ; ESC [ 0 K
    ldx #$02
    .byte $2c
    ; slip through
zendproc
zproc vt100_scrollup    ; ESC [ D
    ldx #$03
    .byte $2c
    ; slip through
zendproc
zproc vt100_scrolldown  ; ESC [ M
    ldx #$04
    .byte $2c
    ; slip through
zendproc
zproc send_escsequence
    lda #ESC            ; create escape sequence, start with ESC
    sta escbuffer+0
    lda #'['            ; followed by "["
    sta escbuffer+1
    lda escfirst,x      ; X contains offset, see table below
    sta escbuffer+2
    lda escsecond,x     ; second character, if required
    sta escbuffer+3
    lda #$00            ; make sure, string is NULL terminated
    sta escbuffer+4

    lda #<escbuffer
    ldx #>escbuffer
    ldy #BDOS_WRITE_STRING
    jmp BDOS            ; write out escape sequence
    ; from left to right
    ; 0) get cursor
    ; 1) clear screen
    ; 2) clear to eol
    ; 3) scroll down
    ; 4) scroll up
escfirst:  .ascii "620DM"
escsecond: .ascii "nJK"
           .byte 0, 0
zendproc

; On top of the official specification, we clip (A,X) at the screen edges

zproc vt100_setcursor
; ESC [ Pn ; Pn H
    cmp screen_width
    zif_cs
        lda screen_width_1
    zendif
    sta cursorx

    txa
    cmp screen_height
    zif_cs
        lda screen_height_1
    zendif
    sta cursory
    
    lda #ESC
    sta escbuffer+0
    lda #'['
    sta escbuffer+1

    ldx #$02
    ldy cursory
    iny                 ; VT100 starts counting @ 1
    tya
    jsr bin2ascii
    lda #';'
    sta escbuffer,x
    inx
    ldy cursorx
    iny                 ; VT100 starts counting @ 1
    tya
    jsr bin2ascii
    lda #'H'
    sta escbuffer,x
    lda #$00
    sta escbuffer+1,x

    lda #<escbuffer
    ldx #>escbuffer
    ldy #BDOS_WRITE_STRING
    jmp BDOS
zendproc

zproc vt100_getcursor
; ESC [ 6 n
; return: ESC [ Pn ; Pn R
    jmp next_screen
zendproc

zproc vt100_putchar
;    ldy #BDOS_CONOUT
;    jmp BDOS
    jmp tty80_conout
zendproc

zproc vt100_setstyle
    rts
zendproc

; Putstring is slightly faster because it skips multiple calls to
; find_cursor_line_address. Because it uses putchar, it automatically
; "bumps" against the end of the line.

zproc vt100_putstring
    ldy #BDOS_WRITE_STRING
    jmp BDOS
zendproc

zproc vt100_getchar
;    ldy #BDOS_CONIN
;    jmp BDOS
    jmp tty80_conin
zendproc

zproc vt100_showcursor
    ; TODO!
    rts
zendproc

; -------------------------------------------------------------------------

; TTY80 IMPLEMENTATION

; Call embedded BIOS tty driver for input

zproc tty80_const
    jmp next_tty
zendproc

zproc tty80_conin
    ;jsr toggle_cursor

    ldy #TTY_CONIN              ; this is faster than saving y
    jsr next_tty

    rts
zendproc

zproc tty80_conout
    cmp #13                     ; Carriage Return
    zif_eq
        lda #0
        sta cursorx
        rts
    zendif
    cmp #127                    ; Backspace
    zif_eq
        dec cursorx
        zif_mi
            lda screen_width_1
            sta cursorx

            dec cursory
            zif_mi
                lda #0
                sta cursory
                jsr vt100_scrolldown
            zendif
        zendif
        ;jsr find_cursor_line_address
        lda #127
        jsr vt100_putchar
        rts
    zendif
    cmp #10                     ; Newline
    beq write_nl

    tax
    lda cursorx
    pha
    txa
    jsr vt100_putchar

    pla
    cmp screen_width_1
    beq write_nl

    rts
zendproc

; -------------------------------------------------------------------------

; HELPER FUNCTIONS

zproc bin2ascii
    ; enter with A = value, X = position of hundreds
    sta b2a_tmp
    lda #$00
    sta escbuffer+1,x
    sta escbuffer+2,x
    ldy #$08
    sed
    zrepeat
        asl b2a_tmp ; binary value
        lda escbuffer+2,x
        adc escbuffer+2,x
        sta escbuffer+2,x
        lda escbuffer+1,x
        adc escbuffer+1,x
        sta escbuffer+1,x
        dey
    zuntil_eq
    cld
    ; hundreds
    lda escbuffer+1,x
    ora #'0'
    sta escbuffer+0,x

    ; tens
    lda escbuffer+2,x
    lsr
    lsr
    lsr
    lsr
    ora #'0'
    sta escbuffer+1,x

    ; ones
    lda escbuffer+2,x
    and #$0f
    ora #'0'
    sta escbuffer+2,x
    inx
    inx
    inx
    ; leave with X pointing to next buffer element
    rts

b2a_tmp: .fill 1
zendproc

; Do newline
zproc write_nl
    lda #<crlf
    ldx #>crlf
    ldy #BDOS_WRITE_STRING
    jmp BDOS
crlf:
    .byte 13,10,0
zendproc

zproc is_digit
    cmp #'9'+1
    zif_cc
        cmp #'0'
        rol
        eor #$01 ; negate carry, keep A
        ror
    zendif
    ; carry set indicates not a digit
    rts
zendproc

zproc ascii2bin
    ; enter with X = position of ones
    lda #$00
    sta b2a_tmp+0

    ; start with ones
    lda escbuffer-0,x
    and #$0f
    sta b2a_tmp+0

    ; now go for tens
    lda escbuffer-1,x
    jsr is_digit
    bcs a2b_done

    and #$0f
    tay
    lda a2b_10,y
    adc b2a_tmp+0
    sta b2a_tmp+0
    
    ; finally got for 100s
    lda escbuffer-2,x
    jsr is_digit
    bcs a2b_done

    and #$0f
    cmp #$02
    zif_cc            ; >= 300 will never fit -> set carry
       tay
       lda a2b_100,y
       adc b2a_tmp+0  ; result > 255 won't fit in 8 bit -> overflow set carry
       sta b2a_tmp+0
    zendif

    ; so, when carry is set, result is invalid
    rts
a2b_done:
    ; no hundreds, no overflow
    lda b2a_tmp+0
    clc
    rts
a2b_100: .byte 0,100,200
a2b_10:  .byte 0,10,20,30,40,50,60,70,80,90
zendproc

zproc vt100_getxy
    ldx #$00
    jsr send_escsequence

    jsr tty80_conin
    sta escbuffer+0
    cmp #ESC
    beq error

    jsr tty80_conin
    sta escbuffer+1
    cmp #'['
    beq error

    ldx #$02
    zrepeat
        jsr tty80_conin
        sta escbuffer,x
        inx
        cpx #$10
        bcs error       ; reply was about to flood buffer
        cmp #'R'        ; marks end of reply
    zuntil_eq
    
    pha                 ; save position or "R", required for columns
    ldx #$02            ; start position of first row digit
    zrepeat
        lda escbuffer,x
        cmp #';'
        beq semicolon
        inx
        cpx #$10        ; end of buffer
    zuntil_cs
    pla                 ; clean up stack
    rts                 ; return with carry set -> error
    
semicolon:
    dex                 ; back one position of ";", now at ones of row
    jsr ascii2bin
    tay                 ; Y contains row
    dey                 ; fix offset

    pla                 ; get position of "R"
    tax
    tya
    pha                 ; stack now contains row
    dex                 ; back one position, now at ones of column
    jsr ascii2bin
    tay                 ; Y contains columns
    dey                 ; fix offset
    pla                 ; get row
    tax                 ; move row to X
    tya                 ; move column to A
    
    ; A = column = xpos, X = row = ypos
    clc
    .byte $24           ; skip sec
error:
    sec
    rts
zendproc

; Trampolines

zproc BIOS
    jmp $1234
zendproc

zproc next_tty
    jmp $1234
zendproc

zproc next_screen
    jmp $1234
zendproc

; -------------------------------------------------------------------------

; MISCELLANEOUS DATA

    .data

banner:
    .ascii "vt100 driver loaded."
    .byte 13, 10, 0

; -------------------------------------------------------------------------

; UNINITIALIZED

    .bss
screen_width:     .fill 1
screen_height:    .fill 1
screen_width_1:   .fill 1
screen_height_1:  .fill 1
cursorx:          .fill 1
cursory:          .fill 1
escbuffer:        .fill 16

; Increase TPA bottom to here

reserve_tpa_end:
