
#include "sound_bus.h"

// for debug only
#include <stdio.h>

#define OVERSAMPLING (8)

static PIO  sound_bus_pio = pio0;
static uint sound_bus_sm  = 0;
extern queue_t sound_bus_queue_cpu_write;

static sound_bus_write_handler_t write_handler;
static sound_bus_read_handler_t  read_handler;


uint8_t sound_bus_read_values[0x100];


void sound_bus_write_handler_default( uint8_t addr, uint8_t data )
{
   // this handler does nothing by intention
   // it just eliminates a null pointer check
}


void sound_bus_write_handler_set( sound_bus_write_handler_t handler )
{
   write_handler = handler;
}


void sound_bus_read_handler_default( uint8_t addr )
{
   // this handler does nothing by intention
   // it just eliminates a null pointer check
}


void sound_bus_read_handler_set( sound_bus_read_handler_t handler )
{
   read_handler = handler;
}


void sound_bus_init()
{
   uint pin;
   const float freq = 1150000;
   uint offset = pio_add_program( pio0, &sound_bus_wait_program );
   pio_sm_config c = sound_bus_wait_program_get_default_config( offset );

   /* setup handlers */
   write_handler = sound_bus_write_handler_default;
   read_handler  = sound_bus_read_handler_default;

   // setup pins
   for( pin = DATABUS0; pin < INPINS; ++pin )
   {
      pio_gpio_init( sound_bus_pio, pin );
   }

   sm_config_set_in_pin_base( &c, DATABUS0 );
   sm_config_set_in_pin_count( &c, INPINS );
   pio_sm_set_consecutive_pindirs( sound_bus_pio, sound_bus_sm,
                                   DATABUS0, INPINS, false );

   sm_config_set_fifo_join( &c, PIO_FIFO_JOIN_RX );
   sm_config_set_in_shift( &c, false, true, INPINS );

   // running the PIO sm 20 times as fast as expected CPU address
   float div = clock_get_hz( clk_sys ) / (freq * OVERSAMPLING);
   sm_config_set_clkdiv( &c, div );

   //pio_sm_set_jmp_pin( sound_bus_pio, sound_bus_sm, CS0 );
   pio_sm_set_jmp_pin( sound_bus_pio, sound_bus_sm, R_W );
   pio_sm_init( sound_bus_pio, sound_bus_sm, offset, &c );
   pio_sm_set_enabled( sound_bus_pio, sound_bus_sm, true );
}


void sound_bus_loop()
{
   uint32_t bus;
   uint8_t  addr;
   for(;;)
   {
//printf( "%s(%d): %d\n", __FILE__, __LINE__, sound_bus_sm );
      bus = pio_sm_get_blocking( sound_bus_pio, sound_bus_sm );
printf( "%s(%d): %05x%s\n", __FILE__, __LINE__, bus, (bus & (1<<R_W)) ? "!" : "" );
      addr = (bus >> ADDRBUS0);
      if( bus & (1 << R_W) )
      {
         /* CPU read */
         gpio_put_masked( 0xFF, sound_bus_read_values[addr] );
         read_handler( addr );
      }
      else
      {
         /* CPU write */
         write_handler( addr, bus & 0xFF );
      }
   }
}

