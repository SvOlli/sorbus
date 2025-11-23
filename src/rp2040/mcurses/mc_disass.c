/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses.h"
#include "../common/disassemble.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRECACHE (16)


typedef union {
   uint16_t       address  : 16;
   struct {
      bool        disass8  :  1;
      bool        disass16 :  1;
   };
} linecache_t;


typedef enum {
   MCD_MODE_FULLOPCODE = 0,
   MCD_MODE_SINGLEBYTE
} mcd_mode_t;


typedef struct {
   mc_disass_t    *dav;
   linecache_t    *linecache;
   uint16_t       lines;
   uint16_t       firstline;
   mcd_mode_t     mode;
} mc_disassemble_t;


static void mcurses_disassemble_alloc( void *d, uint16_t lines )
{
   mc_disassemble_t *mcd = (mc_disassemble_t*)d;

   // add extra lines for scrollback
   lines += PRECACHE;

   if( mcd->linecache )
   {
      if( lines > mcd->lines )
      {
         /* allocated memory so far is no enough */
         mcd->linecache = realloc( mcd->linecache, lines * sizeof(linecache_t) );
      }
   }
   else
   {
      /* nothing allocated so far */
      mcd->linecache = malloc( lines * sizeof(linecache_t) );
   }
   mcd->lines = lines;
   mf_checkheap();
}


static uint16_t mcurses_disassemble_prevaddress( void *d )
{
   mc_disassemble_t  *mcd = (mc_disassemble_t*)d;
   mc_disass_t       *dav = mcd->dav;
   uint16_t l;
   linecache_t *linecache = mcd->linecache;

   switch( mcd->mode )
   {
      case MCD_MODE_SINGLEBYTE:
         return dav->address - 1;
         break;
      case MCD_MODE_FULLOPCODE:
         for( l = 1; l < mcd->lines; ++l )
         {
            if( linecache[l].address == dav->address )
            {
               return linecache[l-1].address;
            }
         }
         break;
      default:
         break;
   }
   return 0;
}


static void mcurses_disassemble_populate( void *d )
{
   mc_disassemble_t *mcd = (mc_disassemble_t*)d;
   mc_disass_t *dav = mcd->dav;
   uint16_t l;
   uint16_t nextaddress = dav->address - PRECACHE;
   uint16_t address = dav->address - PRECACHE;
   linecache_t *linecache = mcd->linecache;

   disass_set_cpu( dav->cpu );
   disass_mx816( dav->m816, dav->x816 );
   for( l = 0; l < mcd->lines; ++l )
   {
      switch( mcd->mode )
      {
         case MCD_MODE_SINGLEBYTE:
            if( address == dav->address )
            {
               mcd->firstline = l;
            }
            if( address == nextaddress )
            {
               linecache[l].disass8 = true;
               linecache[l].address = 1;
               nextaddress += disass_bytes( dav->peek( dav->bank, address ) );
            }
            else
            {
               linecache[l].disass8 = false;
               linecache[l].address = 0;
            }
            ++address;
            break;
         case MCD_MODE_FULLOPCODE:
            if( address <= dav->address )
            {
               mcd->firstline = l;
            }
            linecache[l].address = address;
            {
               uint16_t prevaddress = address - 1;
               if( (dav->peek( dav->bank, prevaddress ) == 0) &&
                   (dav->peek( dav->bank, address ) == 0) )
               {
                  // workaround BRK #$00 is most likely just zeroed RAM
                  address += 1;
               }
               else
               {
                  address += disass_bytes( dav->peek( dav->bank, address ) );
               }
            }
            break;
         default:
            break;
      }
   }
}


static int32_t mcurses_disassemble_move( void *d, int32_t movelines )
{
   mc_disassemble_t *mcd = (mc_disassemble_t*)d;
   mc_disass_t *dav = mcd->dav;
   int32_t l;

   if( movelines )
   {
      switch( mcd->mode )
      {
         case MCD_MODE_SINGLEBYTE:
            dav->address += movelines;
            break;
         case MCD_MODE_FULLOPCODE:
            if( movelines < 0 )
            {
               dav->address = mcurses_disassemble_prevaddress( d );
            }
            else for( l = 0; l < movelines; ++l )
            {
               dav->address += disass_bytes( dav->peek( dav->bank, dav->address ) );
            }
            break;
         default:
            break;
      }
   }

   mcurses_disassemble_populate( d );

   if( movelines == 0 )
   {
      return 0;
   }
   return LINEVIEW_FIRSTLINE;
}


const char* mcurses_disassemble_data( void *d, int32_t offset )
{
   static char output[80];

   mc_disassemble_t *mcd = (mc_disassemble_t*)d;
   mc_disass_t *dav = mcd->dav;
   uint16_t address;
   int next = 0;

   switch( offset )
   {
      case LINEVIEW_FIRSTLINE:
         next += snprintf( &output[0], sizeof(output)-1,
                           "CPU: %-9s Bank: %x (unstable)"
                           , cputype_name( dav->cpu )
                           , dav->bank
                           );
         if( dav->cpu == CPU_65816 )
         {
            next += snprintf( &output[next], sizeof(output)-next-1,
                              " M=%d X=%d"
                              , dav->m816 ? 1 : 0
                              , dav->x816 ? 1 : 0
                              );
         }
         return &output[0];
      case LINEVIEW_LASTLINE:
         return "  Disassembly Viewer  (Ctrl+C to leave)";
      default:
         break;
   }

   switch( mcd->mode )
   {
      case MCD_MODE_SINGLEBYTE:
         address = mcd->linecache[offset+mcd->firstline].address;
         break;
      case MCD_MODE_FULLOPCODE:
      default:
         offset += mcd->firstline;
         address = mcd->linecache[offset].address;
         break;
   }

   if( offset > mcd->lines )
   {
      mcurses_disassemble_alloc( d, offset );
      mcurses_disassemble_populate( d );
   }

   switch( mcd->mode )
   {
      case MCD_MODE_SINGLEBYTE:
         disass_show( DISASS_SHOW_NOTHING );
         snprintf( &output[0], sizeof(output)-1,
                   "$%04X: %02X          ",
                   dav->address + offset,
                   dav->peek( dav->bank, dav->address + offset ) );
         if( address )
         {
            address = dav->address + offset;
            strncat( &output[0], 
                      disass( address,
                              dav->peek( dav->bank, address + 0 ),
                              dav->peek( dav->bank, address + 1 ),
                              dav->peek( dav->bank, address + 2 ),
                              dav->peek( dav->bank, address + 3 )
                            ),
                      sizeof(output)-1 );
         }
         return output;
      case MCD_MODE_FULLOPCODE:
         disass_show( DISASS_SHOW_ADDRESS | DISASS_SHOW_HEXDUMP );
         if( (dav->peek( dav->bank, address + 0 ) == 0) &&
             (dav->peek( dav->bank, address - 1 ) == 0) &&
             (address != dav->address) )
         {
            return disass_brk1( address,
                           dav->peek( dav->bank, address + 0 ),
                           dav->peek( dav->bank, address + 1 ),
                           dav->peek( dav->bank, address + 2 ),
                           dav->peek( dav->bank, address + 3 )
                         );
         }
         else
         {
            return disass( address,
                           dav->peek( dav->bank, address + 0 ),
                           dav->peek( dav->bank, address + 1 ),
                           dav->peek( dav->bank, address + 2 ),
                           dav->peek( dav->bank, address + 3 )
                         );
         }
      default:
         break;
   }
   return "";
}


int32_t mcurses_disassemble_keypress( void *d, uint8_t *ch )
{
   mc_disassemble_t *mcd = (mc_disassemble_t*)d;
   mc_disass_t *dav = mcd->dav;
   int32_t retval = 0;

   switch( *ch )
   {
      case 0x02: // Ctrl+B
         if( ++(dav->bank) > dav->banks() )
         {
            dav->bank = 0;
         }
         retval = LINEVIEW_REDRAWALL;    // force full redraw
         break;
      case 0x07: // Ctrl+G
         move( 1, 1 );
         if( mcurses_get4hex( &(dav->address) ) )
         {
            mcurses_disassemble_populate( d );
            retval = LINEVIEW_FIRSTLINE; // force full redraw
         }
      case 0x0c: // Ctrl+L
         mcurses_disassemble_populate( d );
         retval = LINEVIEW_REDRAWALL;
         break;
      case 0x10: // Ctrl+P (processor)
      case 'P':
      case 'p':
         if( ++(dav->cpu) == CPU_6502RA )
         {
            dav->cpu = CPU_ERROR + 1;
         }
         mcurses_disassemble_populate( d );
         retval = LINEVIEW_REDRAWALL; // force full redraw
         break;
      case 0x01: // Ctrl+A (alternative display: singleline <-> multiline)
      case 'V':
      case 'v':
         mcd->mode = (mcd->mode == MCD_MODE_FULLOPCODE) ? 
                        MCD_MODE_SINGLEBYTE : MCD_MODE_FULLOPCODE;
         mcurses_disassemble_populate( d );
         retval = LINEVIEW_REDRAWALL; // force full redraw
         break;
      case 'M':
      case 'm':
         dav->m816 = !dav->m816;
         mcurses_disassemble_populate( d );
         retval = LINEVIEW_REDRAWALL; // force full redraw
         break;
      case 'X':
      case 'x':
         dav->x816 = !dav->x816;
         mcurses_disassemble_populate( d );
         retval = LINEVIEW_REDRAWALL; // force full redraw
         break;
      default:
         break;
   }

   if( (retval == LINEVIEW_REDRAWALL) || (retval == LINEVIEW_REDRAWDATA) )
   {
      mcurses_disassemble_populate( d );
   }

   return retval; // nothing changed
}


void mcurses_disassemble( mc_disass_t *dav )
{
   lineview_t config     = { 0 };
   mc_disassemble_t  mcd = { 0 };

   mcd.dav           = dav;
   mcd.lines         = screen_get_lines();
   mcd.linecache     = 0;
   mcd.mode          = MCD_MODE_FULLOPCODE;

   config.keypress   = mcurses_disassemble_keypress;
   config.cpos       = 0;
   config.move       = mcurses_disassemble_move;
   config.data       = mcurses_disassemble_data;
   config.d          = (void*)(&mcd);
   config.attributes = MC_ATTRIBUTES_DISASS;

   disass_set_cpu( dav->cpu );
   mcurses_disassemble_alloc( &mcd, mcd.lines );
   mcurses_disassemble_populate( &mcd );
   lineview( &config );

   if( mcd.linecache )
   {
      (void)mf_checkheap();
      free( mcd.linecache );
   }
}
