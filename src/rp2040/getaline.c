/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * vt100 code take from https://github.com/ariadnavigo/sline
 */

#include <pico.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <pico/multicore.h>
#include <pico/platform.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getaline.h"

static queue_t prompt_queue;

typedef enum {
	VT_DEF,
	VT_CHR,
	VT_BKSPC,
	VT_DLT,
	VT_EOF,
	VT_RET,
	VT_TAB,
	VT_UP,
	VT_DWN,
	VT_LFT,
	VT_RGHT,
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
   return 1;
}



static int esc_seq(char *seq)
{
	if( getkey( &seq[0] ) != 1 )
   {
		return -1;
   }
	if( getkey( &seq[1] ) != 1 )
   {
		return -1;
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
		if( esc_seq(&seq[0]) < 0 )
      {
			return VT_DEF;
      }
		if( seq[1] == '3' && seq[2] == '~' )
      {
			return VT_DLT;
      }
		if( (seq[1] == '1' || seq[1] == '7') && seq[2] == '~' )
      {
			return VT_HOME;
      }
		if( (seq[1] == '4' || seq[1] == '8') && seq[2] == '~' )
      {
			return VT_END;
      }

		switch( seq[1] )
      {
         case 'A':
            return VT_UP;
         case 'B':
            return VT_DWN;
         case 'C':
            return VT_RGHT;
         case 'D':
            return VT_LFT;
         case 'H':
            return VT_HOME;
         case 'F':
            return VT_END;
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


char *getaline()
{
   char key;
   vt_key_t ktype;
   static char prompt[32] = { 0 };
   static char input[64] = { 0 };
   int ipos = 0;
   int redraw = 1;
   char clean[] = "\x1b[0K";

   memset( &input[0], 0, sizeof(input) );
   for( ktype = VT_DEF; ktype != VT_RET; )
   {
      ktype = vtgetkey( &key );
      if( ktype != VT_DEF )
      {
         redraw = 1;
      }
      switch( ktype )
      {
         case VT_CHR:
            if( ipos < (sizeof(input) - 1) )
            {
               input[ipos++] = key;
            }
            break;
         case VT_BKSPC:
            if( ipos > 0 )
            {
               input[--ipos] = '\0';
            }
            break;
         case VT_RET:
            printf( "\n\r\x1b[0K" );
            //printf( "" );
            return &input[0];
         default:
            break;
      }
      if( queue_try_remove( &prompt_queue, &prompt[0] ) )
      {
         redraw = 2;
      }
      if( redraw )
      {
         printf( "%c%s%s%s", redraw > 1 ? '\n' : '\r', clean, prompt, input );
         redraw = 0;
      }
   }

   return &input[0];
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
