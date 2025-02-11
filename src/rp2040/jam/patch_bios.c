
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

static uint8_t write_offset = 0xff;
static uint8_t read_offset  = 0xff;

static uint8_t write_code[2][9] = {
   {
      0x2C,0x0F,0xDF,  // : BIT   $DF0F
      0x30,0xFB,       //   BMI   :-
      0x8D,0x0E,0xDF,  //   STA   $DF0E
      0x60             //   RTS
   },                  // 9 bytes
   {
      0x85,0x01,       //   STA   $01
      0x60,            //   RTS
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00
   }
};

static uint8_t read_code[2][12] = {
   {
      0xAD,0x0D,0xDF,  //   LDA   $DF0D
      0xD0,0x02,       //   BNE   :+
      0x38,            //   SEC
      0x60,            //   RTS
      0xAD,0x0C,0xDF,  // : LDA   $DF0C
      0x18,            //   CLC
      0x60             //   RTS
   },                  // 12 bytes
   {
      0x38,            //   SEC
      0xA5,0x01,       //   LDA   $01
      0xF0,0x01,       //   BEQ   :+
      0x18,            //   CLC
      0x60,            // : RTS
      0x00,
      0x00,
      0x00,
      0x00,
      0x00
   }
};


void scanrom( uint8_t *bios )
{
   uint8_t i;
   for( i = 0; i < 0xF0; ++i )
   {
      if( !memcmp( bios+i, &write_code[0][0], 9 ) )
      {
         write_offset = i;
      }
      if( !memcmp( bios+i, &read_code[0][0], 12 ) )
      {
         read_offset = i;
      }
   }
   while( (write_offset == 0xFF) || (read_offset == 0xFF) )
   {
      printf( "  BIOS code for read from / write to UART was modified!\r" );
      sleep_ms(333);
   }
}


void patchrom( uint8_t *bios, uint8_t vgaterm )
{
   vgaterm = vgaterm ? 1 : 0;
   memcpy( bios+write_offset, &write_code[vgaterm][0], 9 );
   memcpy( bios+read_offset,  &read_code[vgaterm][0], 12 );
}
