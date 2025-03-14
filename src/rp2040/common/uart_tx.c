#include "uart_tx.h"
#include "uart_tx.pio.h"
#include <stdio.h>
#include <stdarg.h>
#include <hardware/clocks.h>

static PIO pio;
static uint sm;

static inline void uart_tx_program_init( PIO pio, uint sm, uint offset, uint pin_tx, uint baud )
{
}

static inline void uart_tx_program_putc( PIO pio, uint sm, char c )
{
   pio_sm_put_blocking( pio, sm, (uint32_t)c );
}

static inline void uart_tx_program_puts( PIO pio, uint sm, const char *s )
{
   while( *s )
   {
      uart_tx_program_putc( pio, sm, *(s++) );
   }
}

void uart_tx_init( PIO pio, uint sm, uint pin_tx, uint baud )
{
   uint offset = pio_add_program( pio, &uart_tx_program );

   // Tell PIO to initially drive output-high on the selected pin, then map PIO
   // onto that pin with the IO muxes.
   pio_sm_set_pins_with_mask(    pio, sm, 1u << pin_tx, 1u << pin_tx );
   pio_sm_set_pindirs_with_mask( pio, sm, 1u << pin_tx, 1u << pin_tx );
   pio_gpio_init( pio, pin_tx );

   pio_sm_config c = uart_tx_program_get_default_config( offset );

   // OUT shifts to right, no autopull
   sm_config_set_out_shift( &c, true, false, 32 );

   // We are mapping both OUT and side-set to the same pin, because sometimes
   // we need to assert user data onto the pin (with OUT) and sometimes
   // assert constant values (start/stop bit)
   sm_config_set_out_pins( &c, pin_tx, 1 );
   sm_config_set_sideset_pins( &c, pin_tx );

   // We only need TX, so get an 8-deep FIFO!
   sm_config_set_fifo_join( &c, PIO_FIFO_JOIN_TX );

   // SM transmits 1 bit per 8 execution cycles.
   float div = (float)clock_get_hz(clk_sys) / (8 * baud);
   sm_config_set_clkdiv( &c, div );

   pio_sm_init( pio, sm, offset, &c );
   pio_sm_set_enabled( pio, sm, true );
}

void uart_tx_printf( const char *fmt, ... )
{
   int i;
   char buffer[1024];
   va_list args;
   va_start( args, fmt );
   vsnprintf( &buffer[0], sizeof(buffer-1), fmt, args );
   buffer[sizeof(buffer)-1] = '\0';
   va_end( args );
   for( i = 0; i < sizeof(buffer); ++i )
   {
      if( !buffer[i] )
      {
         break;
      }
      pio_sm_put_blocking( pio, sm, (uint32_t)buffer[i] );
   }
}
