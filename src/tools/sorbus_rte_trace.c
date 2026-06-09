
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "da_trace.h"

/*
 * This will only be used on the host machine
 */
cputype_t text2cputype( const char *text )
{
   const char *c = text;

   /* strip off any leading spaces */
   while( *c == ' ' )
   {
      ++c;
   }

   /* 6502RA before 6502, because otherwise 6502 would match both */
   if( !strncasecmp( text, "6502RA", 6 ) )
   {
      return CPU_6502RA;
   }
   else if( !strncasecmp( text, "6502", 4 ) )
   {
      return CPU_6502;
   }
   else if( !strncasecmp( text, "65C02", 5 ) )
   {
      return CPU_65C02;
   }
   else if( !strncasecmp( text, "65SC02", 6 ) )
   {
      return CPU_65SC02;
   }
   else if( !strncasecmp( text, "65816", 5) )
   {
      return CPU_65816;
   }
   else if( !strncasecmp( text, "65CE02", 6 ) )
   {
      return CPU_65CE02;
   }

   return CPU_ERROR;
}


const char *trace_get_start( char *start, cputype_t *cputype )
{
   char *c;
   const char magic_start[] = "TRACE_START";
   const char magic_end[]   = "TRACE_END";

   /* first check for the end, it's easier */
   c = strstr( start, &magic_end[0] );
   if( !c )
   {
      /* make sure that the end is now marked with a null byte */
      *c = '\0';
   }

   /* now check for start */
   c = strstr( start, &magic_start[0] );
   if( !c )
   {
      /* nothing found: assume data start at file start */
      return start;
   }

   /* advance by number of characters */
   c += sizeof(magic_start)-1;
   /* must be followed by space with cpu variant or newline */
   switch( *c )
   {
      case ' ':
         if( cputype )
         {
            if( *cputype == CPU_ERROR )
            {
               *cputype = text2cputype( c+1 );
            }
         }
         while( *(c++) != '\n' )
         {
            if( !(*c) )
            {
               /* file only contains of start marker */
               return 0;
            }
         }
         return c;
      case '\n':
         return c+1;
      default:
         return 0;
   }
}


da_fullinfo_t *trace_get_fulltrace( const char *start, uint32_t *size )
{
   const char *c;
   uint32_t entries = 0, entry;
   da_fullinfo_t *samples = 0, *sample;

   /* count entried by numbers of lines */
   for( c = start; *(c+1); ++c )
   {
      if( *c == '\n' )
      {
         ++entries;
      }
   }

   /* create memory for trace */
   samples = (da_fullinfo_t*)calloc( entries, sizeof(da_fullinfo_t) );
   sample = samples;

   errno = 0;
   c = start;
   for( entry = 0; entry < entries; ++entry )
   {
      /* make sure that right conversion function is used */
      assert( sizeof(unsigned long long int) == sizeof(da_fullinfo_t) );

      sample->raw = strtoull( c, (char**)&c, 16 );
      if( errno )
      {
         perror( "strtoull" );
         exit( 30 );
      }

      /* skip until the end of the line */
      while( *c != '\n' )
      {
         if( !*(++c) )
         {
            /* leave if we find a null byte */
            break;
         }
      }
      ++sample;
   }

   /* set up return values */
   if( size )
   {
      *size = entries;
   }
   return samples;
}


uint32_t *fulltrace2trace( da_fullinfo_t *refbuffer, uint32_t size )
{
   int i;
   uint32_t *buffer = (uint32_t*)malloc( sizeof(uint32_t) * size );
   uint32_t *u32 = buffer;

   for( i = 0; i < size; ++i )
   {
      *(u32++) = (refbuffer++)->trace;
   }
   return buffer;
}


uint32_t *trace_get_trace( const char *start, uint32_t *size )
{
   uint32_t *retval = 0;
   da_fullinfo_t *fullinfo = trace_get_fulltrace( start, size );
   retval = fulltrace2trace( fullinfo, *size );
   free( fullinfo );
   return retval;
}
