;
; Copyright 2023-2026 SvOlli: Sven Oliver Moll
; Distributed unter the terms: GPL-3.0-or-later
;

; labels used between different parts of the kernel (bank 1)
.global  ascii2bin
.global  bankjmp
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

; entry points for the monitor
.global  mon_init       ; called from reset menu / clean init
.global  mon_restart    ; called in NMOS 6502 toolkit for getting to prompt
.global  mon_irq        ; called via IRQ (either UVNBI or raw on NMOS)
.global  mon_nmi        ; called via NMI
.global  mon_brk        ; called via BRK handler (UVBRK or int MON)
