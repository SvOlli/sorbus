/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mcurses.h"

#include <stdio.h>

#include <pico/stdlib.h>

static uint8_t  mcurses_nodelay   = 0;
static uint16_t mcurses_halfdelay = 0;


uint8_t mcurses_phyio_init()
{
   return 0;
}


void mcurses_phyio_done()
{
}


void mcurses_phyio_putc (uint8_t ch)
{
   putchar( ch );
}


uint8_t mcurses_phyio_getc()
{
   uint32_t timeout_us = 0xFFFFFFFF;
   int ch;

   if( mcurses_halfdelay )
   {
      timeout_us = 100000 * mcurses_halfdelay;  // 10th of seconds
   }
   if( mcurses_nodelay )
   {
      timeout_us = 10;  // 10us timeout is virtually none
   }

   do
   {
      ch = getchar_timeout_us( timeout_us );
   }
   while( (timeout_us == 0xFFFFFFFF) && (ch == PICO_ERROR_TIMEOUT) );

   if( ch == PICO_ERROR_TIMEOUT )
   {
      ch = ERR;
   }
   return ch;
}


void mcurses_phyio_nodelay( uint8_t flag )
{
    mcurses_nodelay = flag;
}


void mcurses_phyio_halfdelay( uint16_t tenths )
{
    mcurses_halfdelay = tenths;
}


void mcurses_phyio_flush_output()
{
}
