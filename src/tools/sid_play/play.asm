!to "play.prg",cbm

; $FF00: read a character from UART in A, returns C=1 when no queue is empty
CHRIN    = $FF00

; $FF03: write a character from A to UART
CHROUT   = $FF03

; $FF06: print a string
; usage:
; jsr PRINT
; .byte "text", 0
; this routine saves all CPU registers, including P, so it can be used for
; debugging messages
PRINT    = $FF06

TMBASE = $DF10     ; timers
TMIMRL = TMBASE+$8 ; repeating IRQ timer low
TMIMRH = TMBASE+$9 ; repeating IRQ timer high, both $00 stop timer
UVNBI  = $DF7C     ; if UVIRQ handles BRK, vector to non-BRK routine

MUSIC_INIT = $1000
MUSIC_PLAY = $1003


    *=$0f80

!if 0 {
    
    sei
    lda #$00
    jsr MUSIC_INIT
-    
    lda $d012
    cmp #$a0
    bne -
    inc $d020
    jsr MUSIC_PLAY
    dec $d020
    jmp -
    
}else{
   sei
   jsr   PRINT
   !byte 10
   !text "Init Song 0"
   !byte 10,0

   
   lda #$00
   jsr MUSIC_INIT
    
   jsr   PRINT
   !byte 10
   !text "setup IRQ"
   !byte 10,0
   
   lda   #<irqhandler
   sta   UVNBI+0
   lda   #>irqhandler
   sta   UVNBI+1

   lda   #200           ; 20.0ms = 50 times per second  , /4 for quad-speed-player
   sta   TMIMRL
   lda   #$00
   sta   TMIMRH

   cli
   
   jsr   PRINT
   !text "play song"
   !byte 10
   !text "ctrl-C to stop"
   !byte 10,0

-
   jsr   CHRIN
   cmp   #$03
   bne   -
   lda   #$00
   sta   TMIMRL
   sta   TMIMRH
   jmp   ($FFFC)        ; reset

irqhandler:
   pha
   txa
   pha
   tya
   pha

   lda   TMIMRL         ; acknoledge timer
   jsr   MUSIC_PLAY          ; play frame

   pla
   tay   
   pla
   tax
   pla
   rti

    
}
    *=$0fff 
    !byte 0
