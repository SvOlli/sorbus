
#include <stdio.h>

#include "gdhelper.h"

#define TILESIZE (16)

void place_circle( gdImagePtr im, int xp, int yp, int color )
{
   gdImageEllipse( im, xp*TILESIZE+TILESIZE/2, yp*TILESIZE+TILESIZE/2,
                   TILESIZE-3, TILESIZE-3, color );
}


void draw_bar_h( gdImagePtr im, int xp, int yp, int len, int color )
{
   gdImageLine( im, xp*TILESIZE+TILESIZE/2-1, yp*TILESIZE+TILESIZE/2-1, (xp+len)*TILESIZE+TILESIZE/2+1, yp*TILESIZE+TILESIZE/2-1, color );
   gdImageLine( im, xp*TILESIZE+TILESIZE/2-1, yp*TILESIZE+TILESIZE/2,   (xp+len)*TILESIZE+TILESIZE/2+1, yp*TILESIZE+TILESIZE/2,   color );
   gdImageLine( im, xp*TILESIZE+TILESIZE/2-1, yp*TILESIZE+TILESIZE/2+1, (xp+len)*TILESIZE+TILESIZE/2+1, yp*TILESIZE+TILESIZE/2+1, color );
}


void draw_bar_v( gdImagePtr im, int xp, int yp, int len, int color )
{
   gdImageLine( im, xp*TILESIZE+TILESIZE/2-1, yp*TILESIZE+TILESIZE/2-1, xp*TILESIZE+TILESIZE/2-1, (yp+len)*TILESIZE+TILESIZE/2+1, color );
   gdImageLine( im, xp*TILESIZE+TILESIZE/2,   yp*TILESIZE+TILESIZE/2-1, xp*TILESIZE+TILESIZE/2,   (yp+len)*TILESIZE+TILESIZE/2+1, color );
   gdImageLine( im, xp*TILESIZE+TILESIZE/2+1, yp*TILESIZE+TILESIZE/2-1, xp*TILESIZE+TILESIZE/2+1, (yp+len)*TILESIZE+TILESIZE/2+1, color );
}


void arrow_right( gdImagePtr im, int xp, int yp, int color )
{
   int i;
   for( i = 0; i < TILESIZE/2; ++i )
   {
      gdImageLine( im, xp*TILESIZE/2-1+i, yp*TILESIZE+i, xp*TILESIZE/2-1+i, (yp+1)*TILESIZE-i, color );
   }
}


int main( int argc, char *argv[] )
{
   int black, white, red;
   int x, y;

   /* image is (32+1)x32 blocks, each block is 12x12 pixels */
   gdImagePtr im = gdImageCreate( TILESIZE*33, TILESIZE*32 );

   black = gdImageColorAllocate( im, 0x00, 0x00, 0x00 );
   white = gdImageColorAllocate( im, 0xff, 0xff, 0xff );
   red   = gdImageColorAllocate( im, 0xff, 0x00, 0x00 );

   gdImageFilledRectangle( im, 0, 0, im->sx, im->sy, white );

   for( y = 0; y < 32; ++y )
   {
      for( x = 0; x < 32; ++x )
      {
         place_circle( im, x+1, y, black );
      }
   }

   draw_bar_v( im, 32, 0, 31, red );
   for( x = 0; x < 31; ++x )
   {
      draw_bar_v( im, 1+x, 0, 15, red );
      draw_bar_v( im, 1+x, 16, 15, red );
   }
   for( x = 0; x < 31; x+=2 )
   {
      draw_bar_h( im, 1+x,  0, 1, red );
      draw_bar_h( im, 1+x, 31, 1, red );
   }
   for( x = 1; x < 31; x+=2 )
   {
      draw_bar_h( im, 1+x, 15, 1, red );
      draw_bar_h( im, 1+x, 16, 1, red );
   }
   draw_bar_h( im, 0, 16, 1, red );
   arrow_right( im, 0, 16, red );

   gdSaveImgGif( im, "WS2812_order.gif" );
   gdImageDestroy( im );

   return 0;
}

