#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "mem_cmds.h"
#include "menu.h"
#include "cpu_detect.h"

extern uint8_t memory[0x10000];  // Need mutex here !!!


uint32_t bank_selected;         // must go to core1.c !!!
bool     bank_enabled;

void bank_adjust( uint32_t bank, uint32_t *addr )
{
   if( bank_enabled && bank )
   {
//      *addr |= 0x10000;
   }
}


/**************************************************************
   Comands for accessing and modifiying native memory
***************************************************************/

void cmd_mem( const char *input )
{
   static uint32_t mem_addr;
   uint32_t mem_start, mem_end;
   uint32_t col;

   if( *input )
   {
      input = get_hex( input, &mem_start, ADDR_DIGITS );
   }
   else
   {
      mem_start = mem_addr;
   }
   input = skip_space( input );
   if( *input )
   {
      get_hex( input, &mem_end, ADDR_DIGITS );
      ++mem_end;
   }
   else
   {
      mem_end = mem_start + 16;
   }
   mem_end &= 0xffff;

   col = 0;
   mem_addr = mem_start;
   do
   //for( mem_addr = mem_start; mem_addr != mem_end; mem_addr = ((mem_addr + 1) & 0xffff) )
   {
      bank_adjust( bank_selected, &mem_addr );
      if( !col )
      {
         printf( ": %04x ", mem_addr );
      }
      printf( " %02x", memory[mem_addr] );
      if( col == 0x07 )
      {
         printf( " " );
      }
      if( col == 0x0F )
      {
         printf( "\n" );
         col = 0;
      }
      else
      {
         ++col;
      }
      mem_addr = ((mem_addr + 1) & 0xffff);
   }
   while(mem_addr != mem_end);
   if( col )
   {
      printf( "\n" );
   }
}


void cmd_colon( const char *input )
{
   uint32_t mem_addr;
   uint32_t value;
   const char *next;

   if( strchr( input, ':' ) )
   {
      // format: addr: values
      input = get_hex( input, &mem_addr, ADDR_DIGITS );
      input = skip_space( input );
      if( *input == ':' )
      {
         // skip colon
         ++input;
      }
      else
      {
         printf( "?: %s\n", input );
         return;
      }
   }
   else
   {
      // format: :addr values
      input = get_hex( input, &mem_addr, ADDR_DIGITS );
   }

   for( ; *input; input = next )
   {
      input = skip_space( input );
      next  = get_hex( input, &value, 2 );
      if( next == input )
      {
         // could not parse input
         printf( "?: %s\n", input );
         return;
      }

      memory[mem_addr++] = value;
      mem_addr &= 0xFFFF;
   }

   return;
}

void cmd_fill( const char *input )
{
   static uint32_t mem_addr;
   uint32_t mem_start, mem_end;
   uint32_t value;

   if( *input )
   {
      input = get_hex( input, &mem_start, ADDR_DIGITS );
   }
   else
   {
      return;
   }
   input = skip_space( input );
   if( *input )
   {
      input = get_hex( input, &mem_end, ADDR_DIGITS );
   }
   else
   {
      return;
   }
   input = skip_space( input );
   if( *input )
   {
      get_hex( input, &value, 2 );
   }
   else
   {
      return;
   }

   for( mem_addr = mem_start; mem_addr != mem_end; mem_addr = ((mem_addr + 1) & 0xffff) )
   {
      bank_adjust( bank_selected, &mem_addr );
      memory[mem_addr] = value;
   }
   bank_adjust( bank_selected, &mem_addr );
   memory[mem_addr] = value;
}

void cmd_bank( const char *input )
{
   cputype_t cputype = get_cpu_type();

   if( cputype != CPU_65816 )
   {
      printf( "warning: bank is only for %s cpu\n", cputype_name( CPU_65816 ) );
   }
   if( !strcasecmp( input, "on" ) )
   {
      bank_enabled = true;
   }
   else if( !strcasecmp( input, "off" ) )
   {
      bank_enabled = false;
   }
   else
   {
      get_hex( input, &bank_selected, 2 );
   }
}
