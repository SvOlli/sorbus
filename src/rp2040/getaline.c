/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * vt100 code taken from https://github.com/ariadnavigo/sline
 */

#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <pico/multicore.h>
#include <pico/platform.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getaline.h"

#define INPUT_SIZE     (64)
#define HISTORY_SIZE   (8)
#ifndef DEBUG_GETALINE
#define DEBUG_GETALINE (0)
#endif

static queue_t prompt_queue;

typedef enum {
   VT_DEF,
   VT_CHR,
   VT_BKSPC,
   VT_DELETE,
   VT_EOF,
   VT_RET,
   VT_TAB,
   VT_UP,
   VT_DOWN,
   VT_LEFT,
   VT_RIGHT,
   VT_HOME,
   VT_END
} vt_key_t;


void getaline_init()
{
   queue_init( &prompt_queue, 32, 16 );
}


int getkey( char *key )
{
   int c;
   c = getchar_timeout_us( 1000 );
   if( c == PICO_ERROR_TIMEOUT )
   {
      return 0;
   }
   *key = c;
#if DEBUG_GETALINE
   printf( "\ngetkey: %02x\n", c );
#endif
   return 1;
}


static int esc_seq( char *seq )
{
   if( getkey( &seq[0] ) != 1 )
   {
      return -1;
   }
   if( getkey( &seq[1] ) != 1 )
   {
      return -1;
   }

   if( seq[0] == 'O' )
   {
      return 0;
   }

   if( seq[0] != '[' )
   {
      return -1;
   }

   if( seq[1] >= '0' && seq[1] <= '9' )
   {
      if( getkey( &seq[2] ) != 1 )
      {
         return -1;
      }
   }

   return 0;
}


vt_key_t vtgetkey( char *retkey )
{
   int r;
   char key;
   char seq[3];

   r = getkey( &key );
   if( !r )
   {
      return VT_DEF;
   }

   *retkey = key;

   if (key == '\x1b')
   {
      if( esc_seq( &seq[0] ) < 0 )
      {
         return VT_DEF;
      }
      if( seq[1] == '3' && seq[2] == '~' )
      {
         return VT_DELETE;
      }
      if( (seq[1] == '1' || seq[1] == '7') && seq[2] == '~' )
      {
         return VT_HOME;
      }
      if( (seq[1] == '4' || seq[1] == '8') && seq[2] == '~' )
      {
         return VT_END;
      }

      switch( seq[0] )
      {
         case '[':
            switch( seq[1] )
            {
               case 'A':
                  return VT_UP;
               case 'B':
                  return VT_DOWN;
               case 'C':
                  return VT_RIGHT;
               case 'D':
                  return VT_LEFT;
            }
         case 'O':
            switch( seq[1] )
            {
               case 'H':
                  return VT_HOME;
               case 'F':
                  return VT_END;
            }
         default:
            return VT_DEF;
      }
   }
   else
   {
      if( key > 0x80 )
      {
         return VT_DEF;
      }
      switch( key )
      {
         case '\x7f':
         case '\x08':
            return VT_BKSPC;
         case '\x04':
            return VT_EOF;
         case '\x0a':
         case '\x0d':
            return VT_RET;
         case '\t':
            return VT_TAB;
         default:
            return VT_CHR;
      }
   }
}


const char *getaline()
{
   char key;
   vt_key_t ktype;
   static char prompt[32] = { 0 };
   static char input[HISTORY_SIZE][INPUT_SIZE] = { 0 };
   static char swapped[INPUT_SIZE] = { 0 };
   int hist = 0;
   int ipos = 0;
   int redraw = 1;
   char clean[] = "\x1b[0K";

   /* only move history when required */
   if( input[0][0] && strncmp( &input[0][0], &input[1][0], INPUT_SIZE ) )
   {
      for( int i = HISTORY_SIZE-1; i > 0; --i )
      {
         memcpy( &input[i][0], &input[i-1][0], INPUT_SIZE );
      }
   }

   /* clean up swapped out line and current line */
   memset( &swapped[0], 0, INPUT_SIZE );
   memset( &input[0][0], 0, INPUT_SIZE );

   for( ktype = VT_DEF; ktype != VT_RET; )
   {
      ktype = vtgetkey( &key );
      if( ktype != VT_DEF )
      {
         redraw = 1;
      }
      switch( ktype )
      {
         case VT_RET:
            /* end of for loop */
            break;
         case VT_CHR:
            if( ipos < (INPUT_SIZE - 1) )
            {
#if 0
               input[0][ipos++] = key;
#else
               for( int i = (INPUT_SIZE - 2); i > ipos; --i )
               {
                  input[0][i] = input[0][i-1];
               }
               input[0][ipos++] = key;
#endif
            }
            break;
         case VT_BKSPC:
            if( ipos > 0 )
            {
#if 0
               input[0][--ipos] = '\0';
#else
               for( int i = ipos; i < (INPUT_SIZE - 1); ++i )
               {
                  input[0][i-1] = input[0][i];
               }
               --ipos;
#endif
            }
            break;
         case VT_DELETE:
            for( int i = ipos+1; i < (INPUT_SIZE - 1); ++i )
            {
               input[0][i-1] = input[0][i];
            }
            break;
         case VT_LEFT:
            if( ipos > 0 )
            {
               --ipos;
            }
            break;
         case VT_RIGHT:
            if( (ipos < (INPUT_SIZE - 1)) && input[0][ipos] )
            {
               ++ipos;
            }
            break;
         case VT_UP:
            if( hist++ == 0 )
            {
               memcpy( &swapped[0], &input[0][0], INPUT_SIZE );
            }
            if( hist >= HISTORY_SIZE )
            {
               hist = HISTORY_SIZE - 1;
            }
            if( !input[hist][0] )
            {
               --hist;
            }
            memcpy( &input[0][0], &input[hist][0], INPUT_SIZE );
            ipos = strlen( &input[0][0] );
            break;
         case VT_DOWN:
            if( --hist < 0 )
            {
               hist = 0;
            }
            if( hist == 0 )
            {
               memcpy( &input[0][0], &swapped[0], INPUT_SIZE );
            }
            else
            {
               memcpy( &input[0][0], &input[hist][0], INPUT_SIZE );
            }
            ipos = strlen( &input[0][0] );
            break;
         case VT_HOME:
            ipos = 0;
            break;
         case VT_END:
            for( ipos = 0; input[0][ipos]; ++ipos )
            {
            }
            break;
#if DEBUG_GETALINE
         case VT_TAB:
            printf( "\nsw: %s\n", swapped );
            for( int i = HISTORY_SIZE-1; i >= 0; --i )
            {
               printf( "%2d: %s\n", i, input[i] );
            }
            printf( "line: %2d\n", hist );
            break;
#endif
         default:
            break;
      }
      if( queue_try_remove( &prompt_queue, &prompt[0] ) )
      {
         redraw = 2;
      }
      if( redraw )
      {
         printf( "%c%s%s%s", redraw > 1 ? '\n' : '\r', clean, prompt, input[0] );
         for( int i = ipos; input[0][i]; ++i )
         {
            printf( "\b" );
         }
         redraw = 0;
      }
   }

   printf( "\n\r\x1b[0K" );
   return &input[0][0];
}


void getaline_prompt( const char *prompt )
{
   queue_try_add( &prompt_queue, &prompt[0] );
}


void getaline_fatal( const char *fmt, ... )
{
   char buffer[256] = { 0 };
   va_list ap;

   va_start( ap, fmt );
   vsnprintf( &buffer[0], sizeof(buffer)-1, fmt, ap );
   va_end( ap );

   printf( "\n" );
   for(;;)
   {
      printf( "\r%s \b", buffer );
      sleep_ms( 1000 );
   }
}
