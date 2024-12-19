
/*
 * small helper program that creates a translation matrix for
 * addressing a ws2812 led panel
 *
 * most probably yours will differ
 */

#include <stdio.h>

int m[32*32];
int t[32*32];

void inverse()
{
   int i;
   for( i = 0; i < 0x400; ++i )
   {
      t[m[i]] = i;
   }
}

#define set(x,y,a) {m[(x)+32*(y)] = a;}

void mktlm1( const char *filename )
{
   FILE *f = fopen( filename, "wb" );
   if( !f )
   {
      fprintf( stderr, "cannot create '%s', running from wrong directory?\n", filename );
      return;
   }
   int a = 0;
   int x, y;
   for( x = 0; x <= 30; x+=2 )
   {
      for( y = 16; y <= 31; ++y )
      {
         set(x+0, y, a++);
      }
      for( y = 31; y >= 16; --y )
      {
         set(x+1, y, a++);
      }
   }
   for( x = 30; x >= 0; x-=2 )
   {
      for( y = 15; y >= 0; --y )
      {
         set(x+1, y, a++);
      }
      for( y = 0; y <= 15; ++y )
      {
         set(x+0, y, a++);
      }
   }

   for( a = 0; a < 32*32; ++a )
   {
      t[m[a]] = a;
   }

   fprintf( f, "const unsigned int translation_matrix[0x0400] = {" );
   for( y = 0; y <= 31; ++y )
   {
      fprintf( f, "\n   " );
      for( x = 0; x <= 31; ++x )
      {
         a = x+y*32;
         fprintf( f, "0x%03x%c",t[a], (a < 1023) ? ',' : '\n' );
      }
   }
   fprintf( f, "};\n" );
   fclose( f );
}

int main()
{
   mktlm1( "src/rp2040/32x32leds/translation_matrix_1.c" );
   return 0;
}
