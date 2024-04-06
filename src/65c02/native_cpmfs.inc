;
; Copyright 2023-2024 Sven Oliver ("SvOlli") Moll
; Distributed unter the terms: GPL-3.0-or-later
;

; usage:
; lda #<filename
; ldx #>filename
; ldy #userpartition
; jsr cpmname
; [...]
; filename:
;    .byte "fname.ext",0
.global  cpmname

; usage:
; jsr cpmname (see above)
; lda #<loadaddress
; sta cpm_saddr+0
; lda #>loadaddress
; sta cpm_saddr+1
; jsr cpmload
.global  cpmload

; usage:
; jsr cpmname (see above)
; lda #<startaddress
; sta cpm_saddr+0
; lda #>startaddress
; sta cpm_saddr+1
; lda #<endaddress (endaddress = last byte to write + 1, Commodore style)
; sta cpm_eaddr+0
; lda #>endaddress
; sta cpm_eaddr+1
; jsr cpmsave
.global  cpmsave

; usage (load to memory):
; lda #<loadaddress
; sta cpm_saddr+0
; lda #>loadaddress
; sta cpm_saddr+1
; ldy #user
; jsr cpmdir
; usage (show on screen)
; lda #$00
; sta cpm_saddr+1
; ldy #user
; jsr cpmdir
.global  cpmdir

; usage:
; jsr cpmname (see above)
; jsr cpmerase
.global  cpmerase

; for providing filename directly
.global  cpm_fname
; for providing start address
.global  cpm_saddr
; for providing end address
.global  cpm_eaddr
