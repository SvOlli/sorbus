#include "uart_tx.h"
#include "uart_tx.pio.h"
#include <stdio.h>
#include <stdarg.h>

static PIO pio;
static uint sm;

void uart_tx_init( PIO pio, uint sm, uint pin, uint baud )
{
   uint offset = pio_add_program( pio, &uart_tx_program );
   uart_tx_program_init( pio, sm, offset, pin, baud );
}

void uart_tx_printf( const char *fmt, ... )
{
   char buffer[1024];
   va_list args;
   va_start( args, fmt );
   vsnprintf( &buffer[0], sizeof(buffer-1), fmt, args );
   buffer[sizeof(buffer)-1] = '\0';
   va_end( args );
   uart_tx_program_puts( pio, sm, &buffer[0] );
}
