#include "picoterm_config.h"
#include "common/uart_tx.h"
#include "string.h"
#include <stdio.h>
#include "picoterm_stddef.h"

// once written, we can access our data at flash_target_contents
const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

picoterm_config_t config; // Issue #13, awesome contribution of Spock64

void debug_print_config( struct PicotermConfig *c );

void load_config(){
  // try to load the config from the Flash memory otherwise initialize a
  // default config.
  if( is_config_in_flash() ){
    uart_tx_printf("load_config() -> read config from Flash\r\n" );
    read_config_from_flash( &config );
    debug_print_config( &config );
  }
  else {
    uart_tx_printf("load_config() -> Set default Config\r\n" );
    set_default_config( &config );
    debug_print_config( &config );
  }
}

void save_config(){
  uart_tx_printf("save_config() -> write config to Flash\r\n" );
  debug_print_config( &config );
  write_config_to_flash( &config );
}

void set_default_config( struct PicotermConfig *c ){
  //c.magic_key = MAGIC_KEY ,
  strncpy( c->magic_key, MAGIC_KEY, sizeof(MAGIC_KEY));
  c->version = CONFIG_VERSION;
  c->colour_preference = WHITE;
  // version 2
  c->baudrate = 115200;
  c->databits = 8;
  c->parity = UART_PARITY_NONE;
  c->stopbits = 1;
	// version 3
	c->font_id = FONT_ASCII; // current font to use
	// version 4
	c->graph_id = FONT_NUPETSCII_MONO8; // prefered graphical font.

}

void upgrade_config( struct PicotermConfig *c ){
  // Upgrade a loaded configuration record

  if( c->version == 1 ){ // Upgrade to version 2 with defaults
    c->baudrate = 115200;
    c->databits = 8;
    c->parity   = UART_PARITY_NONE;
    c->stopbits = 1;
    // Ok for version 2
    c->version  = 2;
	}
	if( c->version == 2 ){ // upgrade to version 3 with defaults
		c->font_id = 0; // ANSI
		// Ok for version 3
		c->version  = 3;
  }
	if( c->version == 3 ){ // Upgrade to version 4 with defaults
		c->graph_id = FONT_NUPETSCII_MONO8;
    // Ok for version 4
    c->version  = 4;
  }
	// Small sanity check
	// if graphical ANSI font activated (in saved data), just override it with
	// the currently graphical ANSI font selected by the user.
	if( c->font_id!=FONT_ASCII )
		c->font_id = c->graph_id;

  /*
  if( c->version == 4 ){ // Upgrade to version 5 with defaults
    // blabla
    c->version  = 5;
  }
  */
}

bool is_config_in_flash(){
  /* Check if the magic key is present in flash */
  struct PicotermConfig _c;
  read_config_from_flash( &_c );
  return strncmp( _c.magic_key, MAGIC_KEY, sizeof(MAGIC_KEY) )==0;
}

void read_config_from_flash( struct PicotermConfig *c ){
    memcpy( c, flash_target_contents, sizeof(struct PicotermConfig) );
    upgrade_config( c );
}

void debug_print_config( struct PicotermConfig *c ){
  uart_tx_printf( "debug_print_config():\r\n" );
  uart_tx_printf( "  magic_key=%s\r\n", c->magic_key );
  uart_tx_printf( "  version=%u\r\n", c->version );
  uart_tx_printf( "  colour_preference=%u\r\n", c->colour_preference );
  uart_tx_printf( "  baudrate=%u\r\n", c->baudrate );
  uart_tx_printf( "  databits=%u\r\n", c->databits );
  uart_tx_printf( "  parity=%u\r\n", c->parity );
  uart_tx_printf( "  stopbits=%u\r\n", c->stopbits );
	uart_tx_printf( "  font_id=%u\r\n", c->font_id );
	uart_tx_printf( "  graph_id=%u\r\n", c->graph_id );
}

void write_config_to_flash( struct PicotermConfig *c ){
   /* unsafe if you have two cores concurrently executing from flash
     https://raspberrypi.github.io/pico-sdk-doxygen/group__hardware__flash.html */
    uart_tx_printf("write_config_to_flash()\r\n");
    debug_print_config( c );
    uint8_t data_to_write[FLASH_PAGE_SIZE];
    memset( data_to_write, 0xFF, FLASH_PAGE_SIZE );
    memcpy( data_to_write, c, sizeof(struct PicotermConfig) );

    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, data_to_write, FLASH_PAGE_SIZE);

}
