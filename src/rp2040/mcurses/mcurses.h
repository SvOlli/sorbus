/*-----------------------------------------------------------------------------
 * mcurses.h - include file for mcurses lib
 *
 * Copyright (c) 2011-2015 Frank Meyer - frank(at)uclock.de
 *
 * Revision History:
 * V1.0 2015 xx xx Frank Meyer, original version
 * V1.1 2017 01 13 ChrisMicro, addepted as Arduino library,
 *                 MCU specific functions removed
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *-----------------------------------------------------------------------------
 * This version has been heavily modified for use with the Sorbus Computer
 * by Sven Oliver Moll. It is intended to work only with the Raspberry Pi
 * PICO-SDK and test cases on the Linux development host.
 *-----------------------------------------------------------------------------
 */

#ifndef __MCURSES_H__
#define __MCURSES_H__ __MCURSES_H__
    
#include "mc_extra.h"


/*------------------------------------------------------------------------------
 * some constants
 *------------------------------------------------------------------------------
 */
#define LINES                   screen_get_lines()
#define COLS                    screen_get_columns()

#ifndef TRUE
#define TRUE                    (1)                                                 // true
#define FALSE                   (0)                                                 // false
#endif

#define OK                      (0)                                                 // yet not used
#define ERR                     (255)                                               // yet not used

/*------------------------------------------------------------------------------
 * attributes, may be ORed
 *------------------------------------------------------------------------------
 */
#define A_NORMAL                0x0000                                              // normal
#define A_UNDERLINE             0x0001                                              // underline
#define A_REVERSE               0x0002                                              // reverse
#define A_BLINK                 0x0004                                              // blink
#define A_BOLD                  0x0008                                              // bold
#define A_DIM                   0x0010                                              // dim
#define A_STANDOUT              A_BOLD                                              // standout (same as bold)

#define F_BLACK                 0x0100                                              // foreground black
#define F_RED                   0x0200                                              // foreground red
#define F_GREEN                 0x0300                                              // foreground green
#define F_BROWN                 0x0400                                              // foreground brown
#define F_BLUE                  0x0500                                              // foreground blue
#define F_MAGENTA               0x0600                                              // foreground magenta
#define F_CYAN                  0x0700                                              // foreground cyan
#define F_WHITE                 0x0800                                              // foreground white
#define F_YELLOW                F_BROWN                                             // some terminals show brown as yellow (with A_BOLD)
#define F_DEFAULT               0x0A00                                              // foreground white
#define F_COLOR                 0x0F00                                              // foreground mask

#define B_BLACK                 0x1000                                              // background black
#define B_RED                   0x2000                                              // background red
#define B_GREEN                 0x3000                                              // background green
#define B_BROWN                 0x4000                                              // background brown
#define B_BLUE                  0x5000                                              // background blue
#define B_MAGENTA               0x6000                                              // background magenta
#define B_CYAN                  0x7000                                              // background cyan
#define B_WHITE                 0x8000                                              // background white
#define B_YELLOW                B_BROWN                                             // some terminals show brown as yellow (with A_BOLD)
#define B_DEFAULT               0xA000                                              // background white
#define B_COLOR                 0xF000                                              // background mask

/*------------------------------------------------------------------------------
 * mcurses variables
 *------------------------------------------------------------------------------
 */
extern uint8_t    mcurses_is_up;                         // flag: mcurses is up
extern uint8_t    mcurses_cury;                          // do not use, use getyx() instead!
extern uint8_t    mcurses_curx;                          // do not use, use getyx() instead!

/*------------------------------------------------------------------------------
 * mcurses functions
 *------------------------------------------------------------------------------
 */
uint8_t           initscr();                             // initialize mcurses
void              move( uint16_t, uint16_t );            // move cursor to line, column (home = 0, 0)
void              attrset( uint16_t );                   // set attribute(s)
void              addch( uint16_t );                     // add a character
void              addstr( const char * );                // add a string
void              getnstr( char * str, uint16_t maxlen );// read a string (with mini editor functionality)
void              setscrreg( uint16_t, uint16_t );       // set scrolling region
void              deleteln();                            // delete line at current line position
void              insertln();                            // insert a line at current line position
void              scroll();                              // scroll line up
void              clear();                               // clear total screen
void              clrtobot();                            // clear screen from current line to bottom
void              clrtoeol();                            // clear from current column to end of line
void              delch();                               // delete character at current position
void              insch( uint16_t );                     // insert character at current position
void              nodelay( uint8_t );                    // set/reset nodelay
void              halfdelay( uint16_t );                 // set/reset halfdelay
uint8_t           getch();                               // read key
void              curs_set( uint8_t );                   // set cursor to: 0=invisible 1=normal 2=very visible
void              refresh();                             // flush output
void              endwin();                              // end mcurses

/*------------------------------------------------------------------------------
 * mcurses macros
 *------------------------------------------------------------------------------
 */
#define erase()                 clear()                                             // clear total screen, same as clear()
#define mvaddch(y,x,c)          move((y),(x)), addch((c))                           // move cursor, then add character
#define mvaddstr(y,x,s)         move((y),(x)), addstr((s))                          // move cursor, then add string



#define mvinsch(y,x,c)          move((y),(x)), insch((c))                           // move cursor, then insert character
#define mvdelch(y,x)            move((y),(x)), delch()                              // move cursor, then delete character
#define mvgetnstr(y,x,s,n)      move((y),(x)), getnstr(s,n)                         // move cursor, then get string
#define getyx(y,x)              y = mcurses_cury, x = mcurses_curx                  // get cursor coordinates

/*------------------------------------------------------------------------------
 * mcurses keys
 *------------------------------------------------------------------------------
 */
#define KEY_TAB                 '\t'                                                // TAB key
#define KEY_CR                  '\r'                                                // RETURN key
#define KEY_BACKSPACE           '\b'                                                // Backspace key
#define KEY_ESCAPE              0x1B                                                // ESCAPE (pressed twice)

#define KEY_DOWN                0x80                                                // Down arrow key
#define KEY_UP                  0x81                                                // Up arrow key
#define KEY_LEFT                0x82                                                // Left arrow key
#define KEY_RIGHT               0x83                                                // Right arrow key
#define KEY_HOME                0x84                                                // Home key
#define KEY_DC                  0x85                                                // Delete character key
#define KEY_IC                  0x86                                                // Ins char/toggle ins mode key
#define KEY_NPAGE               0x87                                                // Next-page key
#define KEY_PPAGE               0x88                                                // Previous-page key
#define KEY_END                 0x89                                                // End key
#define KEY_BTAB                0x8A                                                // Back tab key
#define KEY_F1                  0x8B                                                // Function key F1
#define KEY_F(n)                (KEY_F1+(n)-1)                                      // Space for additional 12 function keys


/* functions to abstract hardware access */
uint8_t mcurses_phyio_init();
void mcurses_phyio_done();
void mcurses_phyio_putc( uint8_t ch );
uint8_t mcurses_phyio_getc();
void mcurses_phyio_nodelay( uint8_t flag );
void mcurses_phyio_halfdelay( uint16_t tenths );
void mcurses_phyio_flush_output ();

#endif
