
#include <unistd.h>

/* this requires the libgd development package */
#include "gdhelper.h"


const unsigned char sx4_code[] = {
   0xa2,0x00,              // ldx   #$00
   0xa9,0x01,              // lda   #$01
   0x8d,0x10,0xd3,         // sta   $d310

   0xbd,0x5a,0x04,         // lda   $045a,x
   0x8d,0x11,0xd3,         // sta   $d311
   0xbd,0x5a,0x05,         // lda   $055a,x
   0x8d,0x12,0xd3,         // sta   $d312
   0xbd,0x5a,0x06,         // lda   $065a,x
   0x8d,0x13,0xd3,         // sta   $d313

   0xbd,0x5a,0x07,         // lda   $075a,x
   0x9d,0x5a,0x07,         // sta   $075a,x
   0xbd,0x5a,0x08,         // lda   $085a,x
   0x9d,0x5a,0x08,         // sta   $085a,x
   0xbd,0x5a,0x09,         // lda   $095a,x
   0x9d,0x5a,0x09,         // sta   $095a,x
   0xbd,0x5a,0x0a,         // lda   $0a5a,x
   0x9d,0x5a,0x0a,         // sta   $0a5a,x

   0xe8,                   // inx
   0xd0,0xd3,              // bne   $0407

   0xa9,0x01,              // lda   #$01
   0x8d,0x0c,0xd3,         // sta   $d30c

   0xa9,0x5a,              // lda   #$5a
   0x8d,0x02,0xd3,         // sta   $d302
   0xa9,0x07,              // lda   #$07
   0x8d,0x03,0xd3,         // sta   $d303

   0x9c,0x06,0xd3,         // stz   $d306
   0x9c,0x07,0xd3,         // stz   $d307
   0xa9,0x1f,              // lda   #$1f
   0x8d,0x08,0xd3,         // sta   $d308
   0x8d,0x09,0xd3,         // sta   $d309
   0x8d,0x0a,0xd3,         // sta   $d30a
   0x9c,0x00,0xd3,         // stz   $d300

   0x6c,0xfc,0xff          // jmp   ($fffc)
};


int main( int argc, char *argv[] )
{
   gdImagePtr im       = 0;
   FILE       *f       = 0;
   int y;

   if( argc != 3 )
   {
      fprintf( stderr, "%s: <infile.gif> <outfile.sx4>\n", argv[0] );
      return 10;
   }
   im = gdLoadImg( argv[1] );
   if( (im->sx != 32) || (im->sy != 32) )
   {
      fprintf( stderr, "image not 32x32 pixels\n" );
      return 22;
   }

   f = fopen( argv[2], "wb" );
   if( !f )
   {
      fprintf( stderr, "cannot create output file\n" );
      return 21;
   }

   fwrite( &sx4_code[0], sizeof(sx4_code), 1, f );

   for( y = 0; y < 0x100; ++y )
   {
      fputc( im->red[y] >> 4, f );
   }
   for( y = 0; y < 0x100; ++y )
   {
      fputc( im->green[y] >> 4, f );
   }
   for( y = 0; y < 0x100; ++y )
   {
      fputc( im->blue[y] >> 4, f );
   }
   for( y = 0; y < im->sy; ++y )
   {
      fwrite( im->pixels[y], im->sx, 1, f );
   }

   fclose( f );
   gdImageDestroy( im );
   return 0;
}
