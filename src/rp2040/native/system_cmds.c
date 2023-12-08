#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "system_cmds.h"
#include "menu.h"
#include "cpu_detect.h"



// This is empty for now. We need another queue to tell core 1 , what we wanna do




static uint32_t requested_cycles = 8;
static uint32_t cycles_left_run = 0;
cputype_t cputype = CPU_UNDEF;

void cmd_reset( const char *input )
{
   if( *input )
   {
      get_dec( input, &requested_cycles );
   }
   else
   {
      requested_cycles = 8;
   }
}


void cmd_nmi( const char *input )
{
   if( *input )
   {
      get_dec( input, &requested_cycles );
   }
   else
   {
      requested_cycles = 8;
   }
}


void cmd_irq( const char *input )
{
   if( *input )
   {
      get_dec( input, &requested_cycles );
   }
   else
   {
      requested_cycles = 8;
   }
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
   if( strlen( input ) )
   {
      return;
   }

   cputype = cpu_detect();
   requested_cycles = 5;
}