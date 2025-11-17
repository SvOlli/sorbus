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


/* basic screen/mcurses functions implemented in mcurses_sorbus.c */
bool screen_get_size( uint16_t *lines, uint16_t *columns );
/* cached values determined by screen_get_size() */
uint16_t screen_get_columns();
uint16_t screen_get_lines();
void screen_save();
void screen_restore();
void screen_alternative_buffer_enable();
void screen_alternative_buffer_disable();
#define SCREEN_TEXT_CENTER ((uint16_t)0xFFFF)
void screen_border( uint16_t top, uint16_t left,
                    uint16_t bottom, uint16_t right );
void screen_textsize( uint16_t *lines, uint16_t *columns,
                      const char *text );
void screen_infobox( uint16_t line, uint16_t column,
                     const char *header, const char *text );
void screen_textbox( uint16_t line, uint16_t column,
                     const char *text );
bool screen_get4hex( uint16_t *value );


/* API for hex editor
 * should be refactored to be more generic, like lineview */
typedef uint8_t(*hexedit_handler_bank_t)();
typedef uint8_t(*hexedit_handler_peek_t)(uint16_t);
typedef void(*hexedit_handler_poke_t)(uint16_t,uint8_t);
typedef struct {
   /* callback functions */
   hexedit_handler_bank_t  nextbank;
   hexedit_handler_peek_t  peek;
   hexedit_handler_poke_t  poke;
   /* initial/return values */
   uint8_t                 bank;
   uint16_t                address;
   uint16_t                topleft;
} hexedit_t;
/* implemented in mcurses_hexedit.c */
void hexedit( hexedit_t *config );


/* keys not handled by lineview:
 * ch as pointer, so it can be modified to overridge keystrokes */
typedef int32_t(*lineview_handler_keypress_t)( void *d, uint8_t *ch );
/* move current view */
typedef int32_t(*lineview_handler_move_t)( void *d, int32_t step );
/* get data for a specific offset from current view */
/* offset is int32_t for LINEVIEW_FIRSTLINE/LINEVIEW_LASTLINE */
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
#define LINEVIEW_FIRSTLINE INT32_MIN
#define LINEVIEW_LASTLINE  INT32_MAX
/* implemented in mcurses_view.c */
void lineview( lineview_t *config );


/* backtrace viewer utilizing lineview */
void mcurses_historian( cputype_t cpu, uint32_t *trace,
                        uint32_t entries, uint32_t start );


/* disassembly viewer utilizing lineview */
typedef uint8_t(*daview_handler_bank_t)();
typedef uint8_t(*daview_handler_peek_t)(uint8_t,uint16_t);
typedef struct {
   /* callback functions */
   daview_handler_bank_t   nextbank;
   daview_handler_peek_t   peek;
   /* initial/return values */
   cputype_t               cpu;
   uint8_t                 bank;
   uint16_t                address;
} daview_t;
void mcurses_disassemble( daview_t *dav );

#endif
