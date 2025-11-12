/**
 * Copyright (c) 2025 SvOlli
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __MCURSES_SORBUS_H__
#define __MCURSES_SORBUS_H__ __MCURSES_SORBUS_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct {
   uint8_t  bank;
   uint16_t address;
   uint16_t topleft;
} hexedit_t;

/* implemented in mcurses_edit.c */
void editLine (char * str, uint8_t lineLength );
int16_t editInt16(int16_t initialNumber);

/* implemented in mcurses_hexedit.c */
void hexedit( hexedit_t *config );

/* callback functions required by hexedit */
uint8_t hexedit_bank();
uint8_t hexedit_peek( uint16_t addr );
void hexedit_poke( uint16_t addr, uint8_t value );

/* implemented in mcurses_sorbus.c */
bool screen_get_size( uint16_t *lines, uint16_t *columns );
void screen_save();
void screen_restore();
void screen_alternative_buffer_enable();
void screen_alternative_buffer_disable();
uint16_t screen_get_columns();
uint16_t screen_get_rows();
void screen_border( uint16_t top, uint16_t left,
                    uint16_t bottom, uint16_t right );
void screen_textbox( uint16_t line, uint16_t column, const char *text );
bool screen_get4hex( uint16_t *value );

#endif
