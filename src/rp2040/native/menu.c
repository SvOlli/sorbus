#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "rp2040_purple.h"
#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <pico/multicore.h>
#include <pico/platform.h>
#include <pico/binary_info.h>

#include <hardware/clocks.h>

#include "menu.h"
#include "mem_cmds.h"
#include "system_cmds.h"
#include "getaline.h"
#include "cpu_detect.h"
#include "mcurses.h"

extern void debug_internal_drive() ;
extern void debug_internal_read_sector(uint16_t dhara_sector);


static bool console_running; 

typedef void (*cmdhandler_t)(const char *input);

typedef struct {
   cmdhandler_t   handler;
   int            textlen;
   const char    *text;
   const char    *help;
} cmd_t;


// forward declaration for array
 void cmd_help( const char *input );
 void cmd_sys( const char *input );
 void cmd_exit( const char *input );

 void cmd_dh_info (const char *input ){

   debug_internal_drive();
 }
 

 void cmd_dh_read (const char *input ){

   uint32_t dhara_sector=0;

   if( *input )
   {
      get_dec( input, &dhara_sector );
   }

   dhara_sector&=0xffff;      // only 65535 sectors allowed

   debug_internal_read_sector((uint16_t)dhara_sector);
 }
 

 cmd_t cmds[] = {
    { cmd_help,   4, "help",   "display help" },
   // { cmd_cold,   4, "cold",   "fully reinitialize system" },
    { cmd_sys,    3, "sys",    "show system information (CPU, flash)" },
   // { cmd_clock,  4, "freq",   "set frequency (dec)" },
   // { cmd_bank,   4, "bank",   "enable (on)/disable (off) 65816 banks, select bank(dec)" },
    { cmd_reset,  5, "reset",  "trigger reset (dec)" },
   // { cmd_format, 6, "format", "format filesystem" },   // This have to be above the "fill" command
   // { cmd_receive, 6, "upload", "upload <FILE> to filesystem" },  
   // { cmd_load,   4, "load",    "load <FILE> from filesystem to memory" },  
   // { cmd_dir,    3, "dir",    "show current directory of filesystem" },
   // { cmd_mkdir,  5, "mkdir",  "creates directory in filesystem" },
   // { cmd_chdir,  5, "chdir",  "(or 'cd') changes directory in filesystem" },
   // { cmd_chdir,  2, "cd",      NULL },
    { cmd_dh_info,  5, "dh_info",    "prints infos about dhara filesystem" },
    { cmd_dh_read, 5, "dh_rd",    "reads and prints sector from dhara filesystem '(dec)" },
    { cmd_irq,    3, "irq",    "trigger maskable interrupt (dec)" },
    { cmd_nmi,    3, "nmi",    "trigger non maskable interrupt (dec)" },
    { cmd_hexedit,    3, "hex",    "open memory window for hexediting (hex)" },

    { cmd_colon,  1, ":",      "write to memory <address> <value> .. (hex)" },
    { cmd_fill,   1, "f",      "fill memory <from> <to> <value> (hex)" },
    { cmd_mem,    1, "m",      "dump memory (<from> (<to>))" },
//    { cmd_steps,  4, "step",   "(or 's') run number of steps (dec)" },          // I always want to type in "step" as a word, not just "s"
//    { cmd_steps,  1, "s",      NULL },
    { cmd_exit,    1, "~",      "leave console ( also 'x' or 'q')" },
    { cmd_exit,    1, "x",      NULL},
    { cmd_exit,    1, "q",      NULL},

    { 0, 0, 0, 0 }
 };
 


char * indent (int indent_len){

   static unsigned char indent_str[]="                   ";
   if (indent_len>=sizeof(indent_str)){
      return "";
   }
   return (&indent_str[indent_len]);

 
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

 
 
void print_welcome(void){
  
  printf("\n\n\n");
  printf("Welcome to Sorbus Native System\n");
  printf("----------------------------\n\n");  
  printf("Compiled on %s %s\n",__DATE__,__TIME__);
  printf("----------------------------\n\n");
  printf("To change to builtin console use '~' key\n");
  printf("in doubt type 'help' on console\n");

}

void cmd_exit( const char *input ){
   printf ("back to native console \n\n");
   console_running=false;
 }
 
void cmd_sys( const char *input )
{
   char cputype_text[16] = { 0 };
   uint f_pll_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
   uint f_pll_usb  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
   uint f_rosc     = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
   uint f_clk_sys  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
   uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
   uint f_clk_usb  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
   uint f_clk_adc  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
   uint f_clk_rtc  = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

   cputype_t cputype = get_cpu_type();

   if( cputype == CPU_UNDEF )
   {
      snprintf( cputype_text, sizeof(cputype_text)-1, "unknown (%02x)", cpu_detect_raw() );
   }
   else
   {
      strncpy( &cputype_text[0], cputype_name(cputype), sizeof(cputype_text)-1 );
   }
   printf("CPU instruction set: %s\n", &cputype_text[0] );
   printf("RP2040 flash size:   16 MB\n");  // make this working

   printf("PLL_SYS:             %3d.%03dMhz\n", f_pll_sys / 1000, f_pll_sys % 1000 );
   printf("PLL_USB:             %3d.%03dMhz\n", f_pll_usb / 1000, f_pll_usb % 1000 );
   printf("ROSC:                %3d.%03dMhz\n", f_rosc    / 1000, f_rosc % 1000 );
   printf("CLK_SYS:             %3d.%03dMhz\n", f_clk_sys / 1000, f_clk_sys % 1000 );
   printf("CLK_PERI:            %3d.%03dMhz\n", f_clk_peri / 1000, f_clk_peri % 1000 );
   printf("CLK_USB:             %3d.%03dMhz\n", f_clk_usb / 1000, f_clk_usb % 1000 );
   printf("CLK_ADC:             %3d.%03dMhz\n", f_clk_adc / 1000, f_clk_adc % 1000 );
   printf("CLK_RTC:             %3d.%03dMhz\n", f_clk_rtc / 1000, f_clk_rtc % 1000 );
}


 
 void cmd_help( const char *input )
 {
    int i;
    for( i = 0; cmds[i].handler; ++i )
    {
      if (cmds[i].help){  
         printf( "%s:%s%s\n",
                 cmds[i].text,
                 indent(cmds[i].textlen - 1),
                 cmds[i].help );
      }
    }
 }

void drawbox (uint8_t y, uint8_t x, uint8_t h, uint8_t w)
{
  uint8_t line;
  uint8_t col;

  move (y, x);
  addch (ACS_ULCORNER);
  for (col = 0; col < w - 2; col++)
  {
    addch (ACS_HLINE);
  }
  addch (ACS_URCORNER);

  for (line = 0; line < h - 2; line++)
  {
    move (line + y + 1, x);
    addch (ACS_VLINE);
    move (line + y + 1, x + w - 1);
    addch (ACS_VLINE);
  }

  move (y + h - 1, x);
  addch (ACS_LLCORNER);
  for (col = 0; col < w - 2; col++)
  {
    addch (ACS_HLINE);
  }
  addch (ACS_LRCORNER);
}


void menu_run()
{
  int i;
   const char *input;

   getaline_init();
   console_running=true;
   clear ();
   drawbox (6, 20, 10, 20);
   
   while(console_running)
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
      // also allow for Apple-like memory input
      // "1234: 56" instead of ":1234 56"
      char *token = strchr( input, ':' );
      if( token )
      {
         cmd_colon( skip_space( input ) );
      }
   }
     
}

