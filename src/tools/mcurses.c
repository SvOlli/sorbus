
#include <fcntl.h>
#include <termio.h>
#include <termios.h>         //termios, TCSANOW, ECHO, ICANON

#include "../rp2040/common/mcurses_sorbus.c"
#include "../rp2040/common/mcurses.c"
#include "../rp2040/common/mcurses_hexedit.c"

uint8_t memory[0x10000] = { 0 };


/*------------------------------------------------------------------------------
 * PHYIO: init, done, putc, getc, nodelay, halfdelay, flush for UNIX or LINUX
 *------------------------------------------------------------------------------
 */
static struct termio                     mcurses_oldmode;
static struct termio                     mcurses_newmode;

/*------------------------------------------------------------------------------
 * PHYIO: init (unix/linux)
 *------------------------------------------------------------------------------
 */
uint8_t mcurses_phyio_init (void)
{
   uint8_t  rtc = 0;
   int      fd;

   fd = fileno (stdin);

   if (ioctl (fd, TCGETA, &mcurses_oldmode) >= 0 || ioctl (fd, TCGETA, &mcurses_newmode) >= 0)
   {
      mcurses_newmode.c_lflag &= ~ICANON;                               // switch off canonical input
      mcurses_newmode.c_lflag &= ~ECHO;                                 // switch off echo
      mcurses_newmode.c_iflag &= ~ICRNL;                                // switch off CR->NL mapping
      mcurses_newmode.c_oflag &= ~TAB3;                                 // switch off TAB conversion
      mcurses_newmode.c_cc[VINTR] = '\377';                              // disable VINTR VQUIT
      mcurses_newmode.c_cc[VQUIT] = '\377';                              // but don't touch VSWTCH
      mcurses_newmode.c_cc[VMIN] = 1;                                  // block input:
      mcurses_newmode.c_cc[VTIME] = 0;                                 // one character

      if (ioctl (fd, TCSETAW, &mcurses_newmode) >= 0)
      {
         rtc = 1;
      }
   }

   return rtc;
}

/*------------------------------------------------------------------------------
 * PHYIO: done (unix/linux)
 *------------------------------------------------------------------------------
 */
void mcurses_phyio_done (void)
{
   int    fd;

   fd = fileno (stdin);

   (void) ioctl (fd, TCSETAW, &mcurses_oldmode);
}

/*------------------------------------------------------------------------------
 * PHYIO: putc (unix/linux)
 *------------------------------------------------------------------------------
 */
void mcurses_phyio_putc (uint8_t ch)
{
   putchar (ch);
}

/*------------------------------------------------------------------------------
 * PHYIO: getc (unix/linux)
 *------------------------------------------------------------------------------
 */
uint8_t mcurses_phyio_getc (void)
{
   uint8_t ch;

   ch = getchar ();

   return (ch);
}

/*------------------------------------------------------------------------------
 * PHYIO: set/reset nodelay (unix/linux)
 *------------------------------------------------------------------------------
 */
void mcurses_phyio_nodelay (uint8_t flag)
{
   int    fd;
   int    fl;

   fd = fileno (stdin);

   if ((fl = fcntl (fd, F_GETFL, 0)) >= 0)
   {
      if (flag)
      {
         fl |= O_NDELAY;
      }
      else
      {
         fl &= ~O_NDELAY;
      }
      (void) fcntl (fd, F_SETFL, fl);
      mcurses_nodelay = flag;
   }
}

/*------------------------------------------------------------------------------
 * PHYIO: set/reset halfdelay (unix/linux)
 *------------------------------------------------------------------------------
 */
void mcurses_phyio_halfdelay (uint8_t tenths)
{
   if (tenths == 0)
   {
      mcurses_newmode.c_cc[VMIN] = 1;        /* block input:     */
      mcurses_newmode.c_cc[VTIME] = 0;       /* one character    */
   }
   else
   {
      mcurses_newmode.c_cc[VMIN] = 0;        /* set timeout      */
      mcurses_newmode.c_cc[VTIME] = tenths;  /* in tenths of sec */
   }

   (void) ioctl (0, TCSETAW, &mcurses_newmode);
}

/*------------------------------------------------------------------------------
 * PHYIO: flush output (unix/linux)
 *------------------------------------------------------------------------------
 */
void mcurses_phyio_flush_output ()
{
   fflush (stdout);
}


uint8_t hexedit_bank()
{
   static uint8_t bank = 0;
   bank = !bank;
   return bank;
}


uint8_t hexedit_peek( uint16_t address )
{
   return memory[address];
}


void hexedit_poke( uint16_t address, uint8_t value )
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


int main( int argc, char *argv[] )
{
   static struct termios oldt, newt;
   uint16_t x, y;
   bool rv;
   hexedit_t config = { 0x00, 0x0000, 0x0000 };

   tcgetattr( STDIN_FILENO, &oldt );
   newt = oldt;
   newt.c_lflag &= ~(ICANON | ECHO);
   tcsetattr( STDIN_FILENO, TCSANOW, &newt );

   screen_save();
   rv = screen_get_size( &y, &x );
   initscr();

   move( 0, 0 );
   clear();
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

   endwin();
   tcsetattr( STDIN_FILENO, TCSANOW, &oldt );

#if 1
   screen_restore();
#else
   printf( "columns=%d rows=%d return=%d\n", x, y, rv );
   printf( "bank=%02x address=%04x topleft=%04x\n",
            config.bank, config.address, config.topleft );
#endif
   return 0;
}
