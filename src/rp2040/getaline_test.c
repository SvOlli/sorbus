/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/platform.h>

#include "getaline.h"


void run_backend()
{
   char spinner[5] = "\\|/-";
   char prompt[32] = "- > ";
   int spos = 0;

   for(;;)
   {
      prompt[0] = spinner[spos++];
      spos &= 0x3;
      getaline_prompt( &prompt[0] );
      sleep_ms(333);
   }
}


void run_console()
{
   const char *bla;
   for(;;)
   {
      bla = getaline();
      printf( "\r[%s]\n", bla );
   }
}


int main()
{
   stdio_init_all();
   uart_set_translate_crlf(uart0, true);

   getaline_init();

   multicore_launch_core1( run_backend );
   run_console();

   return 0;
}
