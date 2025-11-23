/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __MCURSES_SORBUS_H__
#define __MCURSES_SORBUS_H__ __MCURSES_SORBUS_H__

#include <stdbool.h>
#include <stdint.h>

#include "../common/generic_helper.h"

#define  MC_ATTRIBUTES_DISASS    (F_WHITE | B_GREEN)
#define  MC_ATTRIBUTES_HEXEDIT   (F_WHITE | B_RED)
#define  MC_ATTRIBUTES_BACKTRACE (F_BLACK | B_YELLOW)
#define  MC_ATTRIBUTES_XMODEM    (F_WHITE | B_MAGENTA)
#define  MC_ATTRIBUTES_DEFAULT   (F_DEFAULT | B_DEFAULT | A_NORMAL)


/* basic screen/mcurses functions implemented in mcurses_sorbus.c */
bool screen_get_size( uint16_t *lines, uint16_t *columns );
/* cached values determined by screen_get_size() */
uint16_t screen_get_columns();
uint16_t screen_get_lines();
/* send escape sequence to save the current screen */
void screen_save();
/* send escape sequence to restore the previously saved screen */
void screen_restore();
/* send escape sequences for alternative buffer */
void screen_alternative_buffer_enable();
void screen_alternative_buffer_disable();

/* generic functions for getting data on the display */

/* draw a nice Sorbus logo */
void mcurses_sorbus_logo( uint16_t line, uint16_t column );

/* draw a nice border */
void mcurses_border( bool dframe, uint16_t top, uint16_t left,
                     uint16_t bottom, uint16_t right );

/* within this border draw a nice horizontal line */
void mcurses_line_horizontal( bool dframe, uint16_t top, uint16_t left,
                              uint16_t right );

/* within this border draw a nice vertical line */
void mcurses_line_vertical( bool dframe, uint16_t top, uint16_t left,
                            uint16_t bottom );

/* mcurses text line (padded with spaces) */
void mcurses_textline( uint16_t line, uint16_t left, uint16_t right,
                       const char *text );

/* determine the size of a text in lines and (largest) columns */
void mcurses_textsize( uint16_t *lines, uint16_t *columns,
                       const char *text );

/* get a 4 digit hex number from keyboard */
bool mcurses_get4hex( uint16_t *value );

/* output a hex number */
void mcurses_hexout( uint64_t value, uint8_t digits );
#define mcurses_hexout2(v) mcurses_hexout((v),2)
#define mcurses_hexout4(v) mcurses_hexout((v),4)

/* draw a box containing a title and some text */
void mcurses_titlebox( bool dframe, uint16_t line, uint16_t column,
                       const char *title, const char *text );

/* draw a box containing some text */
void mcurses_textbox( bool dframe, uint16_t line, uint16_t column,
                      const char *text );

/* mcurses_textbox and mcurses_infobox can also be centered */
#define MC_TEXT_CENTER ((uint16_t)0xFFFF)


/* API for hex editor
 * should be refactored to be more generic, like lineview */
typedef uint8_t(*hexedit_handler_bank_t)();
typedef struct {
   /* callback functions */
   hexedit_handler_bank_t  nextbank;
   peek_t                  peek;
   poke_t                  poke;
   /* initial/return values */
   uint8_t                 bank;
   uint16_t                address;
   uint16_t                topleft;
} mc_hexedit_t;
/* implemented in mcurses_hexedit.c */
void hexedit( mc_hexedit_t *config );


/* keys not handled by lineview:
 * ch as pointer, so it can be modified to overridge keystrokes */
typedef int32_t(*lineview_handler_keypress_t)( void *d, uint8_t *ch );
/* move current view */
typedef int32_t(*lineview_handler_move_t)( void *d, int32_t step );
/* get data for a specific offset from current view */
/* offset is int32_t for MC_LINEVIEW_FIRSTLINE/MC_LINEVIEW_LASTLINE */
typedef const char*(*lineview_handler_data_t)( void *d, int32_t offset );
/* get cursor position type */
typedef void(*lineview_handler_cpos_t)( void *d,
                                        uint16_t *line, uint16_t *column );
typedef struct {
   /* callback functions */
   lineview_handler_data_t data;
   lineview_handler_move_t move;
   lineview_handler_cpos_t cpos;
   lineview_handler_keypress_t keypress;
   /* data structure handed back to move, bank and data */
   void     *d;
   uint16_t attributes;
} lineview_t;
#define MC_LINEVIEW_FIRSTLINE    INT32_MIN
#define MC_LINEVIEW_LASTLINE     INT32_MAX
#define MC_LINEVIEW_REDRAWDATA   INT32_MIN
#define MC_LINEVIEW_REDRAWALL    INT32_MAX
/* implemented in mcurses_view.c */
void lineview( lineview_t *config );


/* backtrace viewer utilizing lineview */
void mcurses_historian( cputype_t cpu, uint32_t *trace,
                        uint32_t entries, uint32_t start );


/* disassembly viewer utilizing lineview */
typedef uint8_t(*daview_handler_bank_t)();
typedef struct {
   /* callback functions */
   daview_handler_bank_t   banks;
   peek_t                  peek;
   /* initial/return values */
   cputype_t               cpu;
   uint8_t                 bank;
   uint16_t                address;
   bool                    m816;
   bool                    x816;
} mc_disass_t;
void mcurses_disassemble( mc_disass_t *dav );

/* menu for uploading data via xmodem */
bool mc_xmodem_upload( poke_t poke );

#endif
