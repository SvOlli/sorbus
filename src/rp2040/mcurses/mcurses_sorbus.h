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


/* toggle through banks */
typedef uint8_t(*lineview_handler_bank_t)( void *d );
/* move current view */
typedef int32_t(*lineview_handler_move_t)( void *d, int32_t step );
/* get data for a specific offset from current view */
typedef const char*(*lineview_handler_data_t)( void *d, int32_t offset );
typedef struct {
   /* callback functions */
   lineview_handler_bank_t nextbank;
   lineview_handler_move_t move;
   lineview_handler_data_t data;
   /* data structure handed back to move, bank and data */
   void     *d;
   uint16_t attributes;
} lineview_t;

#define LINEVIEW_FIRSTLINE INT32_MIN
#define LINEVIEW_LASTLINE  INT32_MAX

/* implemented in mcurses_view.c */
void lineview( lineview_t *config );


void traceview( cputype_t cpu, uint32_t *trace,
                uint32_t entries, uint32_t start );


typedef uint8_t(*daview_handler_bank_t)();
typedef uint8_t(*daview_handler_peek_t)(uint16_t);
typedef struct {
   /* callback functions */
   daview_handler_bank_t   nextbank;
   daview_handler_peek_t   peek;
   /* initial/return values */
   cputype_t               cputype;
   uint8_t                 bank;
   uint16_t                address;
} daview_t;

void mcurses_historian( cputype_t cpu, uint32_t *trace,
                        uint32_t entries, uint32_t start );

/* implemented in mcurses_sorbus.c */
bool screen_get_size( uint16_t *lines, uint16_t *columns );
void screen_save();
void screen_restore();
void screen_alternative_buffer_enable();
void screen_alternative_buffer_disable();
uint16_t screen_get_columns();
uint16_t screen_get_lines();
void screen_border( uint16_t top, uint16_t left,
                    uint16_t bottom, uint16_t right );
void screen_textbox( uint16_t line, uint16_t column, const char *text );
bool screen_get4hex( uint16_t *value );

#endif
