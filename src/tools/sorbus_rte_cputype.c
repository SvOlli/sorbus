
#include "sorbus_rte.h"

#include <ctype.h>
#include <string.h>

cputype_t getcputype( const char *argi )
{
   cputype_t retval = CPU_ERROR;
   char arg[8] = { 0 };
   const char *c = argi;
   int i;

   /* strip off any leading spaces */
   while( *c == ' ' )
   {
      ++c;
   }

   for( i = 0; *c && (i < 7); ++c, ++i )
   {
      arg[i] = toupper( *c );
   }

   if( !strncasecmp( arg, "6502", 4 ) )
   {
      retval = CPU_6502;
   }
   else if( !strncasecmp( arg, "65C02", 5 ) )
   {
      retval = CPU_65C02;
   }
   else if( !strncasecmp( arg, "65SC02", 6 ) )
   {
      retval = CPU_65SC02;
   }
   else if( !strncasecmp( arg, "65816", 5) )
   {
      retval = CPU_65816;
   }
   else if( !strncasecmp( arg, "65CE02", 6 ) )
   {
      retval = CPU_65CE02;
   }

   return retval;
}
