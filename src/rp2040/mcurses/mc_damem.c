/**
 * Copyright (c) 2025-2026 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses.h"
#include "da_memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRECHECK (16)

/*
 * output mode full opcode:
 * $E000: 4C 03 E0    JMP  $E003
 * "$A: DO y"
 * 
 * output mode single byte:
 * $E000: 4C ('L')    JMP  $E003
 * $E001: 03 ('C')
 * $E002: E0 ('<spades>')
 * "$A: D ('<custom ascii>')    Ey"
 */


static int32_t mcurses_damem_move( void *d, int32_t movelines )
{
   mc_damem_t *mcd = (mc_damem_t*)d;
   da_memory_t dam = mcd->dam;

   if( movelines < 0 )
   {
      da_memory_prev( dam, -movelines );
   }
   else
   {
      da_memory_next( dam, movelines );
   }

   if( movelines == 0 )
   {
      return 0;
   }
   return MC_LINEVIEW_FIRSTLINE;
}


const char* mcurses_damem_data( void *d, int32_t offset )
{
   static char text[128];
   char *b = &text[0];
   size_t bsize = sizeof(text)-1;
   int used = 0;

   mc_damem_t *mcd = (mc_damem_t*)d;
   da_memory_t dam = mcd->dam;
   da_fullinfo_t fullinfo;


   switch( offset )
   {
      case MC_LINEVIEW_FIRSTLINE:
         used += snprintf( b+used, bsize-used,
                           "Bank: %x CPU: ",
                           dam->bank );
         used += da_sn_cpuname( b+used, bsize-used, dam->cpu );
         if( dam->cpu == CPU_65816 )
         {
            used += snprintf( b+used, bsize-used,
                              " M=%d X=%d"
                              , dam->m816 ? 1 : 0
                              , dam->x816 ? 1 : 0
                              );
         }
         return &text[0];
      case MC_LINEVIEW_LASTLINE:
         return "  Disassembly Viewer  (Ctrl+C to leave)";
      default:
         break;
   }

   fullinfo = da_memory_fullinfo( dam, offset );
   if( fullinfo.raw )
   {
      used += da_snf_fullinfo( b+used, bsize-used, dam->cpu,
                               "$A: DO tT cC y", fullinfo, DA_FLAG_NONE );
      return &text[0];
   }

   return "";
}


int32_t mcurses_damem_keypress( void *d, uint8_t *ch )
{
   mc_damem_t *mcd = (mc_damem_t*)d;
   da_memory_t dam = mcd->dam;
   int32_t retval = 0;

   switch( *ch )
   {
      case 0x02: // Ctrl+B
      case 'B':
      case 'b':
         if( ++(dam->bank) > mcd->banks )
         {
            dam->bank = 0;
         }
         retval = MC_LINEVIEW_REDRAWALL;    // force full redraw
         break;
      case 0x07: // Ctrl+G
      case 'G':
      case 'g':
         move( 1, 1 );
         if( mcurses_get4hex( &(dam->address) ) )
         {
            retval = MC_LINEVIEW_FIRSTLINE; // force full redraw
         }
      case 0x0c: // Ctrl+L
         retval = MC_LINEVIEW_REDRAWALL;
         break;
      case 0x10: // Ctrl+P (processor)
      case 'P':
      case 'p':
         if( ++(dam->cpu) == CPU_6502RA )
         {
            ++(dam->cpu);
         }
         if( dam->cpu >= CPU_UNDEF )
         {
            // Rev.A is by definition last, so it can be skipped
            dam->cpu = CPU_ERROR + 1;
         }
         retval = MC_LINEVIEW_REDRAWALL; // force full redraw
         break;
      case 0x16: // Ctrl+V (alternative display: singleline <-> multiline)
      case 'M':
      case 'm':
         if( dam->cpu == CPU_65816 )
         {
            dam->m816 = !dam->m816;
            retval = MC_LINEVIEW_REDRAWALL; // force full redraw
         }
         break;
      case 'X':
      case 'x':
         if( dam->cpu == CPU_65816 )
         {
            dam->x816 = !dam->x816;
            retval = MC_LINEVIEW_REDRAWALL; // force full redraw
         }
         break;
      default:
         break;
   }

   return retval; // nothing changed
}


void mcurses_damem( mc_damem_t *mcd )
{
   lineview_t config = { 0 };
   da_memory_t dam   = mcd->dam;

   config.keypress   = mcurses_damem_keypress;
   config.cpos       = 0;
   config.move       = mcurses_damem_move;
   config.data       = mcurses_damem_data;
   config.d          = (void*)(mcd);
   config.attributes = MC_ATTRIBUTES_DISASS;
   config.charset    = 1;

   da_memory_linecache( dam, screen_get_lines() );
   lineview( &config );
}
