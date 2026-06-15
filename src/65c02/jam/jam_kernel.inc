;
; Copyright 2023-2026 SvOlli: Sven Oliver Moll
; Distributed unter the terms: GPL-3.0-or-later
;

; labels used between different parts of the kernel (bank 1)
.global  ascii2bin
.global  bin2ascii
.global  chrinuc
.global  copybios
.global  gensine        ; obsolete?
.global  inputline
.global  print
.global  prhex4
.global  prhex8
.global  prhex16
.global  printtmp16
.global  sweet16
.global  timstart
.global  uppercase
.global  vt100
.global  wozstart

.if 0
; labels used as destinations for bankjsrs/bankjmps into bank 2
.global  b2browser
.global  b2fb32x32
.global  b2gensine
.global  b2run6502asm

; labels used as destinations for bankjmps into bank 3
.global  b3basic

; labels used as destinations for bankjmps into bank 4
.global  f4forth
.endif

; entry points for the monitor
.global  mon_init       ; called from reset menu / clean init
.global  mon_restart    ; called in NMOS 6502 toolkit for getting to prompt
.global  mon_irq        ; called via IRQ (either UVNBI or raw on NMOS)
.global  mon_nmi        ; called via NMI
.global  mon_brk        ; called via BRK handler (UVBRK or int MON)

; function index for ldx #$00 : jmp bankjmp
;B2BASIC   = B2BROWSER+1
