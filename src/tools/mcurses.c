
#include <fcntl.h>
#include <termio.h>
#include <termios.h>         //termios, TCSANOW, ECHO, ICANON
#include <stdio.h>
#include <unistd.h>

#if 0
#include "../rp2040/mcurses/mcurses_sorbus.c"
#include "../rp2040/mcurses/mcurses.c"
#include "../rp2040/mcurses/mcurses_hexedit.c"
#include "../rp2040/mcurses/mcurses_view.c"
#endif
#include "../rp2040/mcurses/mcurses.h"

uint8_t memory[0x10000] = { 0 };


/*
 *****************************************************************************
 * hexedit callbacks
 *****************************************************************************
 */
uint8_t _hexedit_bank()
{
   static uint8_t bank = 0;
   bank = !bank;
   return bank;
}


uint8_t _hexedit_peek( uint16_t address )
{
   return memory[address];
}


void _hexedit_poke( uint16_t address, uint8_t value )
{
   memory[address] = value;
}

const char *speeds[] = {
   "CLK_SYS:", "133.000MHz",
   "CLK_PERI:", "48.000MHz",
   "CLK_65C02:", "1.181816MHz",
   NULL
};


const char *debug_info_heap()
{
   static char buffer[80] = { 0 };

   uint32_t total_heap = 150596;
   uint32_t free_heap  = 149104;

   snprintf( &buffer[0], sizeof(buffer)-1,
             "total heap: %06x (%d)\n"
             "free  heap: %06x (%d)"
             , total_heap, total_heap, free_heap,  free_heap );

   return &buffer[0];
}


static int32_t lv_offset = 0;

bool lv_move( int32_t lines )
{
   if( ((lv_offset + lines) < 0) || ((lv_offset + lines) > 1000) )
   {
      return false;
   }
   lv_offset += lines;
   return true;
}


const char *lv_data( int32_t line )
{
   static char buffer[256];
   switch( line )
   {
      case LINEVIEW_FIRSTLINE:
         return "title";
      case LINEVIEW_LASTLINE:
         return "footer";
      default:
         break;
   }
   snprintf( &buffer[0], sizeof(buffer)-1,
             "line: %d", line + lv_offset );
   return &buffer[0];
}

int main( int argc, char *argv[] )
{
   static struct termios oldt, newt;
   uint16_t x, y;
   bool rv;
   hexedit_t config = { _hexedit_bank, _hexedit_peek, _hexedit_poke, 0x00, 0x0000, 0x0000 };
   lineview_t lvconfig = { NULL, lv_move, lv_data, F_WHITE | B_BLUE, 0, 0 };

   tcgetattr( STDIN_FILENO, &oldt );
   newt = oldt;
   newt.c_lflag &= ~(ICANON | ECHO);
   tcsetattr( STDIN_FILENO, TCSANOW, &newt );

   screen_save();
   rv = screen_get_size( &y, &x );
   initscr();

   move( 0, 0 );
   clear();
#if 1
   lineview( &lvconfig );
#else
   screen_border( 0, 0, y-1, x-1 );
#if 0
   screen_table( 2, 2, speeds );
#else
   screen_textbox( 2, 2, debug_info_heap() );
#endif


   getch();

#if 0
   hexedit( &config );
#endif
#endif

   endwin();
   tcsetattr( STDIN_FILENO, TCSANOW, &oldt );

#if 1
   screen_restore();
   printf( "columns=%d rows=%d return=%d\n", x, y, rv );
#else
   printf( "bank=%02x address=%04x topleft=%04x\n",
            config.bank, config.address, config.topleft );
#endif
   return 0;
}
