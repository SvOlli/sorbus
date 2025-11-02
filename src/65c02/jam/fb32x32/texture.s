; -*- mode: c; tab-width: 4; fill-column: 128 -*-
; vi: set ts=4 tw=128:

; Texture, Copyright (c) 2017 Dave Odell <dmo2118@gmail.com>
;
; Permission to use, copy, modify, distribute, and sell this software and its
; documentation for any purpose is hereby granted without fee, provided that
; the above copyright notice appear in all copies and that both that
; copyright notice and this permission notice appear in supporting
; documentation.  No representations are made about the suitability of this
; software for any purpose.  It is provided "as is" without express or
; implied warranty.

; A port of 20 year old QBasic code to a much more modern platform.

.include "fb32x32.inc"

sorbusinit:
   lda   #$00
   sta   $10
   ldx   #$cc
   stx   $11
   ldy   #$01
   int   FB32X32

   sei
   lda   #FB32X32_CMAP_C64
   sta   FB32X32_COLMAP
   lda   #<irqhandler
   sta   UVNBI+0
   lda   #>irqhandler
   sta   UVNBI+0
   lda   #$90
   sta   TMIMRL
   lda   #$01
   sta   TMIMRH
   cli

sorbusdone:
   lda   RANDOM
   sta   src_row

   ldy   #0
top_loop:
   lda   RANDOM
   and   #$f
   sbc   #$8
   adc   src_row,y
   iny
   sta   src_row,y
   cpy   #31
   bne   top_loop

init_loop:
   jsr   tex
   clc
   lda   $10
   adc   #$20
   sta   $10
   lda   $11
   adc   #0
   sta   $11
   cmp   #$d0
   bne   init_loop

   lda   #$e0
   sta   $10
   lda   #$cf
   sta   $11

loop:
   ;jmp skip_blit

   lda   $ff
:
   cmp   $ff
   beq   :-

   clc
   lda   #0
blit_loop2:
   tay
   lda   $cc20,y
   sta   $cc00,y
   lda   $cc21,y
   sta   $cc01,y
   lda   $cc22,y
   sta   $cc02,y
   lda   $cc23,y
   sta   $cc03,y
   lda   $cc24,y
   sta   $cc04,y
   lda   $cc25,y
   sta   $cc05,y
   lda   $cc26,y
   sta   $cc06,y
   lda   $cc27,y
   sta   $cc07,y

   tya
   adc   #8
   bne   blit_loop2

   clc
   lda #0
blit_loop3:
   tay
   lda   $cd20,y
   sta   $cd00,y
   lda   $cd21,y
   sta   $cd01,y
   lda   $cd22,y
   sta   $cd02,y
   lda   $cd23,y
   sta   $cd03,y
   lda   $cd24,y
   sta   $cd04,y
   lda   $cd25,y
   sta   $cd05,y
   lda   $cd26,y
   sta   $cd06,y
   lda   $cd27,y
   sta   $cd07,y

   tya
   adc   #8
   bne   blit_loop3

   clc
   lda   #0
blit_loop4:
   tay
   lda   $ce20,y
   sta   $ce00,y
   lda   $ce21,y
   sta   $ce01,y
   lda   $ce22,y
   sta   $ce02,y
   lda   $ce23,y
   sta   $ce03,y
   lda   $ce24,y
   sta   $ce04,y
   lda   $ce25,y
   sta   $ce05,y
   lda   $ce26,y
   sta   $ce06,y
   lda   $ce27,y
   sta   $ce07,y

   tya
   adc   #8
   bne   blit_loop4

   lda   #0
blit_loop5:
   tay
   lda   $cf20,y
   sta   $cf00,y
   lda   $cf21,y
   sta   $cf01,y
   lda   $cf22,y
   sta   $cf02,y
   lda   $cf23,y
   sta   $cf03,y
   lda   $cf24,y
   sta   $cf04,y
   lda   $cf25,y
   sta   $cf05,y
   lda   $cf26,y
   sta   $cf06,y
   lda   $cf27,y
   sta   $cf07,y

   tya
   clc
   adc   #8
   cmp   #$e0
   bne   blit_loop5

skip_blit:

   jsr   tex
   jmp   loop

tex:
   lda   RANDOM
   and   #$f
   sbc   #$8
   adc   src_row
   sta   src_row

   ldy   #0
   tax
   lda   pal0,x
   sta   ($10),y
tex_loop0:
   lda   RANDOM
   and   #$f
   sbc   #$8
   ;clc
   adc   src_row,y
   iny
   ;clc
   adc   src_row,y
   ror
   sta   src_row,y

   tax
   lda   pal0,x
   sta   ($10),y

   cpy   #31
   bne   tex_loop0
   rts

irqhandler:
   stz   FB32X32_COPY
   bit   TMIMRL
   inc   $ff
   rti

pal0:
   .byte $00, $00, $00, $00, $00, $00, $00, $00
   .byte $00, $00, $0b, $0b, $0b, $0b, $0b, $0b
   .byte $0b, $0b, $0b, $0b, $0b, $0b, $0b, $0b
   .byte $0c, $0c, $0c, $0c, $0c, $0c, $0c, $0c
   .byte $0c, $0c, $0c, $0c, $0c, $0c, $0c, $0c
   .byte $0f, $0f, $0f, $0f, $0f, $0f, $0f, $0f
   .byte $0f, $0f, $0f, $0f, $0f, $0f, $0f, $0f
   .byte $0f, $01, $01, $01, $01, $01, $01, $01
   .byte $01, $01, $01, $01, $01, $01, $01, $01
   .byte $0f, $0f, $0f, $0f, $0f, $0f, $0f, $0f
   .byte $0f, $0f, $0f, $0f, $0f, $0f, $0f, $0f
   .byte $0f, $0c, $0c, $0c, $0c, $0c, $0c, $0c
   .byte $0c, $0c, $0c, $0c, $0c, $0c, $0c, $0c
   .byte $0c, $0b, $0b, $0b, $0b, $0b, $0b, $0b
   .byte $0b, $0b, $0b, $0b, $0b, $0b, $0b, $00
   .byte $00, $00, $00, $00, $00, $00, $00, $00
   .byte $00, $00, $00, $00, $00, $00, $00, $00
   .byte $00, $00, $00, $00, $00, $00, $00, $00
   .byte $0b, $0b, $0b, $0b, $0b, $0b, $0b, $0b
   .byte $0b, $0b, $0b, $0b, $0b, $0b, $0b, $0b
   .byte $0b, $0b, $0b, $0b, $0b, $0b, $05, $05
   .byte $05, $05, $05, $05, $05, $05, $05, $05
   .byte $05, $05, $05, $05, $05, $05, $05, $05
   .byte $0d, $0d, $0d, $0d, $0d, $0d, $0d, $0d
   .byte $0d, $0d, $0d, $0d, $0d, $0d, $0d, $0d
   .byte $0d, $05, $05, $05, $05, $05, $05, $05
   .byte $05, $05, $05, $05, $05, $05, $05, $05
   .byte $05, $05, $05, $0b, $0b, $0b, $0b, $0b
   .byte $0b, $0b, $0b, $0b, $0b, $0b, $0b, $0b
   .byte $0b, $0b, $0b, $0b, $0b, $0b, $0b, $0b
   .byte $0b, $00, $00, $00, $00, $00, $00, $00
   .byte $00, $00, $00, $00, $00, $00, $00, $00

src_row:
   ; (32 bytes)

; A full-resolution version of the same thing. Not perhaps the most interesting thing to look at.

;#include "screenhack.h"
;
;#include <inttypes.h>
;
;struct texture
;{
;   unsigned width, height;
;   Colormap colormap;
;   GC gc;
;   unsigned long palette[128];
;   XImage *image;
;   uint8_t *row;
;   unsigned graininess;
;   unsigned lines;
;};
;
;#define GRAIN(self) (NRAND((self)->graininess * 2 + 1) - (self)->graininess)
;
;static void _put_line(struct texture *self, Display *dpy, Window window, unsigned y)
;{
;   unsigned x;
;   for(x = 0; x != self->width; ++x)
;   {
;   unsigned c1 = self->row[x];
;   unsigned c0 = c1;
;   if(c0 & 64)
;      c0 ^= 127;
;   XPutPixel(self->image, x, 0, self->palette[(c0 & 63) | ((c1 & 0x80) >> 1)]);
;   }
;
;   XPutImage(dpy, window, self->gc, self->image, 0, 0, 0, y, self->width, 1);
;
;   *self->row += GRAIN(self);
;
;   for(x = 1; x != self->width; ++x);
;   {
;   unsigned avg = self->row[x - 1] + self->row[x];
;   self->row[x] = ((avg + ((avg & 2) >> 1)) >> 1) + GRAIN(self);
;   }
;
;
;}
;
;static void texture_reshape(Display *dpy, Window window, void *self_raw, unsigned w, unsigned h)
;{
;   struct texture *self = self_raw;
;   unsigned x, y;
;
;   if(w == self->width && h == self->height)
;   return;
;
;   self->image->bytes_per_line = 0;
;   self->image->width = w;
;   XInitImage(self->image);
;
;   free(self->row);
;   self->row = malloc(w);
;   free(self->image->data);
;   self->image->data = malloc(w * self->image->bytes_per_line);
;
;   self->width = w;
;   self->height = h;
;
;   *self->row = NRAND(0xff);
;   for(x = 1; x != self->width; ++x)
;   self->row[x] = self->row[x - 1] + GRAIN(self);
;
;   for(y = 0; y != self->height; ++y)
;   _put_line(self, dpy, window, y);
;}
;
;static void *texture_init(Display *dpy, Window window)
;{
;   static const XGCValues gcv_src = {GXcopy};
;   XGCValues gcv = gcv_src;
;   struct texture *self = malloc(sizeof(*self));
;   XWindowAttributes xwa;
;   unsigned i;
;
;   XGetWindowAttributes(dpy, window, &xwa);
;   self->width = 0;
;   self->height = 0;
;   self->colormap = xwa.colormap;
;
;   self->graininess = get_integer_resource(dpy, "graininess", "Graininess");
;   self->lines = get_integer_resource(dpy, "speed", "Speed");
;
;   for(i = 0; i != 64; ++i)
;   {
;   XColor color;
;   unsigned short a = i * (0x10000 / 64);
;
;   color.red   = a;
;   color.green = a;
;   color.blue  = a;
;   if(!XAllocColor(dpy, xwa.colormap, &color))
;      abort();
;   self->palette[i] = color.pixel;
;
;   color.red   = 0;
;   color.green = a;
;   color.blue  = 0;
;   if(!XAllocColor(dpy, xwa.colormap, &color))
;      abort();
;   self->palette[i | 64] = color.pixel;
;   }
;
;   self->gc = XCreateGC(dpy, window, GCFunction, &gcv);
;   self->image = XCreateImage(dpy, xwa.visual, xwa.depth, ZPixmap, 0, NULL, 0, 1, 32, 0);
;   self->row = NULL;
;
;   texture_reshape(dpy, window, self, xwa.width, xwa.height);
;
;   return self;
;}
;
;static unsigned long texture_draw(Display *dpy, Window window, void *self_raw)
;{
;   struct texture *self = self_raw;
;   unsigned y;
;   XCopyArea(dpy, window, window, self->gc, 0, self->lines, self->width, self->height - self->lines, 0, 0);
;   for(y = 0; y != self->lines; ++y)
;   _put_line(self, dpy, window, self->height - self->lines + y);
;   return 16667;
;}
;
;static Bool texture_event (Display *dpy, Window window, void *self_raw, XEvent *event)
;{
;   return False;
;}
;
;static void texture_free(Display *dpy, Window window, void *self_raw)
;{
;   struct texture *self = self_raw;
;
;   XFreeGC(dpy, self->gc);
;   XFreeColors(dpy, self->colormap, self->palette, 128, 0);
;   XDestroyImage(self->image);
;   free(self->row);
;   free(self);
;}
;
;static const char *texture_defaults[] =
;{
;   "*speed:      2",
;   "*graininess: 1",
;   "*fpsSolid:   True",
;   "*fpsTop:     True",
;   0
;};
;
;static XrmOptionDescRec texture_options[] =
;{
;   {"-speed",      ".speed",      XrmoptionSepArg, 0},
;   {"-graininess", ".graininess", XrmoptionSepArg, 0},
;   {0, 0, 0, 0}
;};
;
;XSCREENSAVER_MODULE("Texture", texture)
