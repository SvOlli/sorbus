/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "rp2040_purple.h"
#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <pico/multicore.h>
#include <pico/platform.h>
#include <pico/binary_info.h>
#include <hardware/flash.h>

#include <hardware/clocks.h>

bi_decl(bi_program_name("Sorbus Computer Monitor Command Prompt"))
bi_decl(bi_program_description("make the Sorbus Computer a tool for learning about the 65C02/65816/6502 CPU"))
bi_decl(bi_program_url("https://xayax.net/sorbus/"))

#include "bus.h"
#include "cpu_detect.h"
#include "disassemble.h"
#include "payload_mcp.h"
#include "getaline.h"

#define MEM_ACCESS 0
#define SHOW_CLOCK 0
#define SHOW_RAW_BUS 0

#define BUF_SIZE 64
#define INIT_STR NULL /* Set to a literal string to test init strings */

#define STEP_DELAY 1

/* 128k of memory for 65816: 64k bank 0 + 64k for bank > 1, mirrored */
/* 65(C)02 does not have bank address, can only access bank 0 */
uint8_t  memory[0x20000];
#define  ADDR_DIGITS (4)
uint32_t bank_selected     = 0;
bool     bank_enabled      = false;
bool     disass_enabled    = false;

uint32_t cycles_left_reset = 6;
uint32_t cycles_left_nmi   = 0;
uint32_t cycles_left_irq   = 0;
uint32_t cycles_left_run   = 1;
uint32_t add_us            = 100000;

uint32_t state;
uint32_t address;

cputype_t cputype = CPU_UNDEF;

typedef void (*cmdhandler_t)(const char *input);

typedef struct {
   cmdhandler_t   handler;
   int            textlen;
   const char    *text;
   const char    *help;
} cmd_t;


uint8_t memcheck[] = {
   0xD8,
   0xA2, 0x00,
   0x8A,
   0x0a,
   0x69, 0x00,
   0x0a,
   0x69, 0x00,
   0x0a,
   0x69, 0x00,
   0x0a,
   0x69, 0x00,
   0x9D, 0x00, 0x10,
   0xE8,
   0xD0, 0xED,
   0xF0, 0xFE
};


uint32_t flash_size_detect()
{
   uint32_t size;
   /* sizes:    1MB               16MB */
   for( size = (1 << 20); size < (1 << 25); size <<= 1 )
   {
      const uint8_t *flash_base = (const uint8_t *)XIP_BASE;
      if( !memcmp( flash_base, flash_base + size, FLASH_SECTOR_SIZE ) )
      {
         break;
      }
   }

   return size;
}


void bank_adjust( uint32_t bank, uint32_t *addr )
{
   if( bank_enabled && bank )
   {
      *addr |= 0x10000;
   }
}


void set_freq( uint32_t freq )
{
   add_us = 1000000 / (freq << 1);
}


void bus_delay()
{
   static uint64_t next_us = 0;

   while( next_us > time_us_64() )
   {
      tight_loop_contents();
   }

   next_us += add_us;
}


const char *skip_space( const char *input )
{
   while( *input == ' ' )
   {
      ++input;
   }
   return input;
}


const char *get_dec( const char *input, uint32_t *value )
{
   char *retval;

   if( !value || !input )
   {
      return 0;
   }

   input = skip_space( input );
   *value = strtoul( input, &retval, 10 );
   return retval;
}


const char *get_hex( const char *input, uint32_t *value, uint32_t digits )
{
   char *retval;
   char buffer[9];

   if( !value || !input || (digits < 1) || (digits > 8) )
   {
      return 0;
   }

   input = skip_space( input );

   strncpy( &buffer[0], input, digits );
   buffer[digits] = '\0';

   *value = strtoul( &buffer[0], &retval, 16 );

   return input + (retval-&buffer[0]);
}


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


void cmd_reset( const char *input )
{
   if( *input )
   {
      get_dec( input, &cycles_left_reset );
   }
   else
   {
      cycles_left_reset = 8;
   }
}


void cmd_nmi( const char *input )
{
   if( *input )
   {
      get_dec( input, &cycles_left_nmi );
   }
   else
   {
      cycles_left_nmi = 8;
   }
}


void cmd_irq( const char *input )
{
   if( *input )
   {
      get_dec( input, &cycles_left_irq );
   }
   else
   {
      cycles_left_irq = 8;
   }
}


void cmd_sys( const char *input )
{
   uint f_pll_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
   uint f_pll_usb  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
   uint f_rosc     = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
   uint f_clk_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
   uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
   uint f_clk_usb  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
   uint f_clk_adc  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
   uint f_clk_rtc  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

   printf("CPU instruction set: %s\n", cputype_name( cputype ) );
   printf("RP2040 flash size:   %dMB\n", flash_size_detect() / (1 << 20) );

   printf("PLL_SYS:             %3d.%03dMhz\n", f_pll_sys / 1000, f_pll_sys % 1000 );
   printf("PLL_USB:             %3d.%03dMhz\n", f_pll_usb / 1000, f_pll_usb % 1000 );
   printf("ROSC:                %3d.%03dMhz\n", f_rosc    / 1000, f_rosc % 1000 );
   printf("CLK_SYS:             %3d.%03dMhz\n", f_clk_sys / 1000, f_clk_sys % 1000 );
   printf("CLK_PERI:            %3d.%03dMhz\n", f_clk_peri / 1000, f_clk_peri % 1000 );
   printf("CLK_USB:             %3d.%03dMhz\n", f_clk_usb / 1000, f_clk_usb % 1000 );
   printf("CLK_ADC:             %3d.%03dMhz\n", f_clk_adc / 1000, f_clk_adc % 1000 );
   printf("CLK_RTC:             %3d.%03dMhz\n", f_clk_rtc / 1000, f_clk_rtc % 1000 );
}


void cmd_clock( const char *input )
{
   uint32_t freq;
   if( *input )
   {
      get_dec( input, &freq );
      if( (freq < 1) || (freq > 250000) )
      {
         return;
      }
      add_us = 500000 / freq;
   }
   printf( "freq: %6d Hz\n", 500000 / add_us );
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


void cmd_steps( const char *input )
{
   uint32_t c = cycles_left_run;
   get_dec( input, &cycles_left_run );
   if( (cycles_left_run < 1) || (cycles_left_run > 100000) )
   {
      // if there were any steps left, stop by setting to 0,
      // else go a single step
      cycles_left_run = c ? 0 : 1;
   }
}


void cmd_cold( const char *input )
{
   bool debug = false;
   if( strlen( input ) )
   {
      if( !strcmp( input, "debug" ) )
      {
         debug = true;
      }
      else
      {
         return;
      }
   }
   cputype = cpu_detect( debug );

   cycles_left_reset = 5;
   disass_cpu( cputype );
   disass_show( DISASS_SHOW_NOTHING );
   printf( "CPU detected as %s\n", cputype_name( cputype ) );
}


void cmd_dis( const char *input )
{
   if( !strcasecmp( input, "on" ) )
   {
      disass_enabled = true;
   }
   else if( !strcasecmp( input, "off" ) )
   {
      disass_enabled = false;
   }
}


void cmd_bank( const char *input )
{
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


// forward declaration for array
void cmd_help( const char *input );


cmd_t cmds[] = {
   { cmd_help,   4, "help",   "display help" },
   { cmd_cold,   4, "cold",   "fully reinitialize system (\"debug\")" },
   { cmd_sys,    3, "sys",    "show system information (CPU, flash)" },
   { cmd_clock,  4, "freq",   "set frequency (dec)" },
   { cmd_dis,    3, "dis",    "enable (on)/disable (off) automated disassembly" },
   { cmd_bank,   4, "bank",   "enable (on)/disable (off) 65816 banks, select bank(dec)" },
   { cmd_reset,  5, "reset",  "trigger reset (dec)" },
   { cmd_irq,    3, "irq",    "trigger maskable interrupt (dec)" },
   { cmd_nmi,    3, "nmi",    "trigger non maskable interrupt (dec)" },
   { cmd_colon,  1, ":",      "write to memory <address> <value> .. (hex)" },
   { cmd_fill,   1, "f",      "fill memory <from> <to> <value> (hex)" },
   { cmd_mem,    1, "m",      "dump memory (<from> (<to>))" },
   { cmd_steps,  1, "s",      "run number of steps (dec)" },
   { 0, 0, 0, 0 }
};


void cmd_help( const char *input )
{
   int i;
   char *indent = "      ";
   for( i = 0; cmds[i].handler; ++i )
   {
      printf( "%s:%s%s\n",
              cmds[i].text,
              indent+cmds[i].textlen - 1,
              cmds[i].help );
   }
}


void run_bus()
{
   uint8_t bank;
   for(;;)
   {
      if( cycles_left_run > 0 )
      {
         gpio_set_mask( bus_config.mask_rdy );

         if( cycles_left_reset )
         {
            --cycles_left_reset;
            gpio_clr_mask( bus_config.mask_reset );
         }
         else
         {
            gpio_set_mask( bus_config.mask_reset );
         }

         if( cycles_left_nmi )
         {
            --cycles_left_nmi;
            gpio_clr_mask( bus_config.mask_nmi );
         }
         else
         {
            gpio_set_mask( bus_config.mask_nmi );
         }

         if( cycles_left_irq )
         {
            --cycles_left_irq;
            gpio_clr_mask( bus_config.mask_irq );
         }
         else
         {
            gpio_set_mask( bus_config.mask_irq );
         }

         --cycles_left_run;
      }
      else
      {
         gpio_clr_mask( bus_config.mask_rdy );
      }

      // done: wait for the right time to set clock to high
      bus_delay();
      gpio_set_dir_in_masked( bus_config.mask_data );
      // wait for the RP2040 to adjust
      sleep_us(1);
	  // for 65816 only: read bank address
      bank = (gpio_get_all() >> bus_config.shift_data); // truncate is intended;
      gpio_set_mask( bus_config.mask_clock );

      // bus should be still valid from clock low
      state = gpio_get_all();
      address = ((state & bus_config.mask_address) >> bus_config.shift_address);

      // setup bus direction
      if( state & bus_config.mask_rw )
      {
         // read from memory and write to bus
         gpio_set_dir_out_masked( bus_config.mask_data );
         bank_adjust( bank, &address );
         gpio_put_masked( bus_config.mask_data, ((uint32_t)memory[address]) << bus_config.shift_data );
#if MEM_ACCESS
         printf( "mem r %04x->%02x\r", address, memory[address] );
#endif
      }
      else
      {
         // read from bus and write to memory write
         gpio_set_dir_in_masked( bus_config.mask_data );
         // wait for the RP2040 to adjust
         sleep_us(1);
         bank_adjust( bank, &address );
         memory[address] = (gpio_get_all() >> bus_config.shift_data); // truncate is intended;
#if MEM_ACCESS
         printf( "mem w %04x<-%02x\r", address, memory[address] );
#endif
      }

      // done: wait for the right time to set clock to low
      bus_delay();
      state = gpio_get_all();

      if( state & bus_config.mask_rdy )
      {
         char buffer[64] = { 0 };
         char bankhex[3] = { 0 };
         if( bank_enabled )
         {
            snprintf( &bankhex[0], sizeof(bankhex), "%02x", bank & 0xFF, 3 );
            bankhex[sizeof(bankhex)-1] = '\0';
         }
         if( disass_enabled )
         {
            address = ((state & bus_config.mask_address) >> bus_config.shift_address);
            snprintf( &buffer[0], sizeof(buffer), "%3d:%s%04x %c %02x %c%c%c %-15s>",
               cycles_left_run > 999 ? 999 : cycles_left_run,
               bankhex,
               (state & bus_config.mask_address) >> (bus_config.shift_address),
               (state & bus_config.mask_rw) ? 'r' : 'w',
               (state & bus_config.mask_data) >> (bus_config.shift_data),
               (state & bus_config.mask_reset) ? ' ' : 'R',
               (state & bus_config.mask_nmi) ? ' ' : 'N',
               (state & bus_config.mask_irq) ? ' ' : 'I',
               disass( address,
                       memory[(address + 0) & 0xFFFF],
                       memory[(address + 1) & 0xFFFF],
                       memory[(address + 2) & 0xFFFF],
                       memory[(address + 3) & 0xFFFF] ) );
         }
         else
         {
            snprintf( &buffer[0], sizeof(buffer), "%3d:%s%04x %c %02x %c%c%c>",
               cycles_left_run > 999 ? 999 : cycles_left_run,
               bankhex,
               (state & bus_config.mask_address) >> (bus_config.shift_address),
               (state & bus_config.mask_rw) ? 'r' : 'w',
               (state & bus_config.mask_data) >> (bus_config.shift_data),
               (state & bus_config.mask_reset) ? ' ' : 'R',
               (state & bus_config.mask_nmi) ? ' ' : 'N',
               (state & bus_config.mask_irq) ? ' ' : 'I' );
         }
         buffer[sizeof(buffer)-1] = '\0';
         getaline_prompt( buffer );
      }
      gpio_clr_mask( bus_config.mask_clock );
   }
}


void run_console()
{
   int i;
   const char *input;

   for(;;)
   {
      input = skip_space( getaline() );

      for( i = 0; cmds[i].handler; ++i )
      {
         if( !strncmp( input, cmds[i].text, cmds[i].textlen ) )
         {
            cmds[i].handler( skip_space( input+cmds[i].textlen ) );
            break;
         }
      }
      if( !strncmp( input, "step", 4 ) )
      {
         // I always want to type in "step" as a word, not just "s"
         cmd_steps( skip_space( input+4 ) );
      }
      else
      {
         // also allow for Apple-like memory input
         // "1234: 56" instead of ":1234 56"
         char *token = strchr( input, ':' );
         if( token )
         {
            cmd_colon( skip_space( input ) );
         }
      }
   }
}


int main()
{
   stdio_init_all();
   uart_set_translate_crlf(uart0, true);

   memset( &memory[0], 0xEA, 0x10000 );
   memcpy( &memory[0x300], &memcheck[0], sizeof(memcheck) );
   memcpy( &memory[0x400], &payload_mcp[0], sizeof(payload_mcp) );

   bus_init();
   getaline_init();
   cmd_cold( "" );

   multicore_launch_core1( run_bus );
   run_console();

   return 0;
}
