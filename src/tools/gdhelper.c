
#include <stdio.h>
#include <stdlib.h>

/* this requires the libgd development package */
#include "gdhelper.h"


gdImagePtr gdLoadImg( const char *filename )
{
   gdImagePtr im = 0;
   FILE *f;

   f = fopen( filename, "rb" );
   if( !f )
   {
      fprintf( stderr, "cannot read input file\n" );
      exit( 21 );
   }
   im = gdImageCreateFromPng( f );
   if( !im )
   {
      rewind( f );
      im = gdImageCreateFromGif( f );
   }
   fclose( f );
   if( gdImageTrueColor( im ) )
   {
      fprintf( stderr, "true color image detected, trying conversion: " );
      if( !gdImageTrueColorToPalette( im, 1, 250 ) )
      {
         fprintf( stderr, "ERROR: conversion failed!\n" );
         exit( 20 );
      }
      fprintf( stderr, "done.\n" );
   }
   return im;
}


void gdSaveImgGif( gdImagePtr im, const char *filename )
{
   FILE *f;

   f = fopen( filename, "wb" );
   if( f )
   {
      gdImageGif( im, f );
      fclose( f );
   }
}


void gdSaveImgPng( gdImagePtr im, const char *filename )
{
   FILE *f;

   f = fopen( filename, "wb" );
   if( f )
   {
      gdImagePng( im, f );
      fclose( f );
   }
}


void gdSaveImgBin( gdImagePtr im, const char *filename )
{
   FILE *f;

   f = fopen( filename, "wb" );
   if( f )
   {
      int x,y;
      for( y = 0; y < im->sy; ++y )
      {
         for( x = 0; x < im->sx; ++x )
         {
            fputc( im->pixels[y][x], f );
         }
      }
      fclose( f );
   }
}
