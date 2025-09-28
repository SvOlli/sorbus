
#ifndef __GDHELPER_H__
#define __GDHELPER_H__ __GDHELPER_H__

#include <gd.h>

/* can load png and gif, autodetects by trying both */
gdImagePtr gdLoadImg( const char *filename );
void gdSaveImgGif( gdImagePtr im, const char *filename );
void gdSaveImgPng( gdImagePtr im, const char *filename );
/* does not save colorpalette */
void gdSaveImgBin( gdImagePtr im, const char *filename );

#endif
