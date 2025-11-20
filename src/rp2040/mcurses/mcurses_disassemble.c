/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses.h"
#include "../common/disassemble.h"

#include <stdlib.h>

#define PRECACHE (16)


#if 0
typedef struct {
   /* callback functions */
   daview_handler_bank_t   banks;
   daview_handler_peek_t   peek;
   /* initial/return values */
   cputype_t               cputype;
   uint8_t                 bank;
   uint16_t                address;
} daview_t;
#endif

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
   daview_t       *dav;
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

   if( !(mcd->linecache) )
   {
      mcd->linecache = malloc( mcd->lines * sizeof(linecache_t) );
      mf_checkheap();
   }
   else if( lines > mcd->lines )
   {
      if( mcd->lines < lines )
      {
         mcd->lines = lines;
      }
      mcd->linecache = realloc( mcd->linecache, mcd->lines * sizeof(linecache_t) );
      mf_checkheap();
   }
}


static uint16_t mcurses_disassemble_prevaddress( void *d )
{
   mc_disassemble_t *mcd = (mc_disassemble_t*)d;
   daview_t *dav = mcd->dav;
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
   daview_t *dav = mcd->dav;
   uint16_t l;
   uint16_t nextaddress = dav->address - PRECACHE;
   uint16_t address = dav->address - PRECACHE;
   linecache_t *linecache = mcd->linecache;

fprintf( stderr, "populate: %04x %s\n", address, mcd->mode ? "singlebyte" : "fullopcode" );
   for( l = 0; l < mcd->lines; ++l )
   {
fprintf( stderr, "%2d: %04x %04x ", l, address, nextaddress );
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
fprintf( stderr, "%04x\n", linecache[l].address );
   }
}


static int32_t mcurses_disassemble_move( void *d, int32_t movelines )
{
   mc_disassemble_t *mcd = (mc_disassemble_t*)d;
   daview_t *dav = mcd->dav;
   int32_t l;

fprintf( stderr, "move: %d\n", movelines );
   switch( mcd->mode )
   {
      case MCD_MODE_SINGLEBYTE:
         dav->address += movelines;
         break;
      case MCD_MODE_FULLOPCODE:
         if( !movelines )
         {
         }
         else if( movelines < 0 )
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

   mcurses_disassemble_populate( d );

   if( movelines == 0 )
   {
      return 0;
   }
   if( movelines < 0 )
   {
      return LINEVIEW_FIRSTLINE;
   }
   return 1;
}


int32_t mcurses_disassemble_keypress( void *d, uint8_t *ch )
{
   mc_disassemble_t *mcd = (mc_disassemble_t*)d;
   daview_t *dav = mcd->dav;
   switch( *ch )
   {
      case 0x02: // Ctrl+B
         if( ++(dav->bank) > dav->banks() )
         {
            dav->bank = 0;
         }
         mcurses_disassemble_populate( d );
         return LINEVIEW_FIRSTLINE;    // force full redraw
         break;
      case 0x07: // Ctrl+G
         move( 1, 1 );
         if( screen_get4hex( &(dav->address) ) )
         {
            mcurses_disassemble_populate( d );
            return LINEVIEW_FIRSTLINE; // force full redraw
         }
      case 0x10: // Ctrl+P (processor)
         if( ++(dav->cpu) == CPU_UNDEF )
         {
            dav->cpu = CPU_ERROR + 1;
         }
         mcurses_disassemble_populate( d );
         return LINEVIEW_FIRSTLINE; // force full redraw
         break;
      case 0x01: // Ctrl+A (alternative display: singleline <-> multiline)
         mcd->mode = (mcd->mode == MCD_MODE_FULLOPCODE) ? 
                        MCD_MODE_SINGLEBYTE : MCD_MODE_FULLOPCODE;
         mcurses_disassemble_populate( d );
         return LINEVIEW_FIRSTLINE; // force full redraw
         break;
      default:
         break;
   }

   return 0; // nothing changed
}


const char* mcurses_disassemble_data( void *d, int32_t offset )
{
   static char output[80];

   mc_disassemble_t *mcd = (mc_disassemble_t*)d;
   daview_t *dav = mcd->dav;
   uint16_t address;

   switch( offset )
   {
      case LINEVIEW_FIRSTLINE:
         return "(debug)";
      case LINEVIEW_LASTLINE:
         return "  Disassembly Viewer  (Ctrl+C to leave)";
      default:
         break;
   }

   offset += mcd->firstline;
   address = mcd->linecache[offset].address;
fprintf( stderr, "data: %2d: %04x\n", offset, address );
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
             (dav->peek( dav->bank, address - 1 ) == 0) )
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


void mcurses_disassemble( daview_t *dav )
{
   lineview_t config           = { 0 };
   mc_disassemble_t mcd = { 0 };

   mcd.dav           = dav;
   mcd.lines         = screen_get_lines();
   mcd.linecache     = 0;
   mcd.mode          = MCD_MODE_FULLOPCODE;

   config.keypress   = mcurses_disassemble_keypress;
   config.cpos       = 0;
   config.move       = mcurses_disassemble_move;
   config.data       = mcurses_disassemble_data;
   config.d          = (void*)(&mcd);
   config.attributes = F_WHITE | B_GREEN;

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
