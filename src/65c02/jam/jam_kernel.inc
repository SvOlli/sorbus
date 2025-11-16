;
; Copyright 2023-2024 Sven Oliver ("SvOlli") Moll
; Distributed unter the terms: GPL-3.0-or-later
;

; labels used between different parts of the kernel
.global  timstart
.global  wozstart
.global  print
.global  prhex4
.global  prhex8
.global  prhex16
.global  copybios
.global  uppercase
.global  chrinuc
.global  vt100
.global  bin2ascii
.global  ascii2bin
.global  loader
.global  copybios
.global  gensine
.global  printtmp16
.global  inputline
.global  ramio
.global  ramio_cmd

; entry points for the monitor
.global  mon_init       ; called from reset menu / clean init
.global  mon_restart    ; called in NMOS 6502 toolkit for getting to prompt
.global  mon_irq        ; called via IRQ (either UVNBI or raw on NMOS)
.global  mon_nmi        ; called via NMI
.global  mon_brk        ; called via BRK handler (UVBRK or int MON)
