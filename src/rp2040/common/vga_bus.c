
#include "vga_bus.h"

static PIO  vga_bus_pio;
static uint vga_bus_sm;
extern queue_t vga_bus_queue_cpu_write;

static void vga_bus_irq_handler()
{
   uint32_t bus;
   //
   bus = pio_sm_get_blocking( vga_bus_pio, vga_bus_sm );
   queue_try_add( &vga_bus_queue_cpu_write, &bus );
   // acknoledge interrupt
   pio_interrupt_clear( vga_bus_pio, vga_bus_sm );
}

void vga_bus_handler_program_init( PIO pio, uint sm, uint offset,
                                   vga_bus_callback_t callback, float freq )
{
   // other values don't make sense
   const uint startpin    = 18; // D7
   const uint outpins     =  8;
   const uint inpins      = 11; // to get A0 as well
   const uint rwpin       = 26;

   vga_bus_pio = pio;
   vga_bus_sm  = sm;

   pio_gpio_init( pio, startpin );

   // setup pins
   pio_sm_config c = vga_bus_handler_program_get_default_config( offset );
   sm_config_set_in_pin_base( &c, startpin );
   sm_config_set_in_pin_count( &c, inpins );
   sm_config_set_out_pin_base( &c, startpin );
   sm_config_set_out_pin_count( &c, outpins );
   // start with data direction set to read, extra pins are up to A0
   pio_sm_set_consecutive_pindirs( pio, sm, startpin, inpins, false );

   sm_config_set_fifo_join( &c, PIO_FIFO_JOIN_NONE );
   sm_config_set_in_shift( &c, false, true, inpins );
   sm_config_set_out_shift( &c, false, true, outpins );

   // running the PIO sm 20 times as fast as expected CPU address
   float div = clock_get_hz( clk_sys ) / (freq * 20);
   sm_config_set_clkdiv( &c, div );

   pio_sm_set_jmp_pin( pio, sm, rwpin );
   pio_sm_init( pio, sm, offset, &c );
   pio_sm_set_enabled( pio, sm, true );
}

// returns bus as 0x00-0xff: data + 0x400 when A0 set
uint32_t vga_bus_read()
{
   return (pio_sm_get_blocking( vga_bus_pio, vga_bus_sm ) >> 18) & 0x4FF;
}

void vga_bus_write( uint8_t data )
{
   pio_sm_put_blocking( vga_bus_pio, vga_bus_sm, data );
}
