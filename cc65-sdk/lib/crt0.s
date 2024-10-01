; ---------------------------------------------------------------------------
; crt0.s
; ---------------------------------------------------------------------------
;
; Startup code for cc65 (Single Board Computer version)

.export   _init, _exit
.import   _main

.export   __STARTUP__ : absolute = 1        ; Mark as startup
.import   __MAIN_START__, __MAIN_SIZE__     ; Linker generated

.import    copydata, zerobss, initlib, donelib

.include  "zeropage.inc"
.include  "native.inc"
.include  "native_bios.inc"

; ---------------------------------------------------------------------------
; Place the startup code in a special segment

.segment  "STARTUP"

; ---------------------------------------------------------------------------
; A little light 6502 housekeeping

_init:
   ldx   #$FF                    ; Initialize stack pointer to $01FF
   txs
   cld                           ; Clear decimal mode

   int   COPYBIOS                ; Make sure BIOS routines are available
   
   stz   BANK                    ; Switch to RAM bank

; ---------------------------------------------------------------------------
; Set cc65 argument stack pointer

   lda   #<(__MAIN_START__ + __MAIN_SIZE__)
   sta   sp
   lda   #>(__MAIN_START__ + __MAIN_SIZE__)
   sta   sp+1

; ---------------------------------------------------------------------------
; Initialize memory storage

   jsr   zerobss                 ; Clear BSS segment
   jsr   copydata                ; Initialize DATA segment
   jsr   initlib                 ; Run constructors

; ---------------------------------------------------------------------------
; Call main()

   jsr   _main

; ---------------------------------------------------------------------------
; Back from main (this is also the _exit entry):  force a software break

_exit:
   jsr   donelib                 ; Run destructors
   jmp   ($FFFC)

