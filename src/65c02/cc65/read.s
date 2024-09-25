;
; based upon libsrc/atmos/read.s
;
; 2014-08-22, Greg King
;
; int read (int fd, void* buf, unsigned count);
;
; This function is a hack!  It lets us get text from the stdin console.
;

        .export         _read
.if 0
        .constructor    initstdin
.endif

        .import         popax, popptr1
        .importzp       ptr1, ptr2, ptr3
;        .forceimport    disable_caps

        .macpack        generic
        .include        "../native_bios.inc"

.proc   _read
        cpx     #$01
        bcs     @inputoverflow
        cmp     #$50    ; 79 chars max
        bcc     @inputok
@inputoverflow:
        lda     #$4f
@inputok:
        pha

        jsr     popptr1         ; get buf
        jsr     popax           ; get fd and discard

        lda     #$00
        tay
        sta     (ptr1),y        ; make sure that buffer is empty

        ply
        lda     ptr1+0
        ldx     ptr1+1
        int     LINEINPUT

; No error, return count.

L9:     tya
        ldx     #$00
        rts

.endproc

.if 0
;--------------------------------------------------------------------------
; initstdin:  Reset the stdin console.

.segment        "ONCE"

initstdin:
        ldx     #<-1
        stx     text_count
        rts


;--------------------------------------------------------------------------

.segment        "INIT"

text_count:
        .res    1
.endif
