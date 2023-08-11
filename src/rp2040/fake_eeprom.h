/**
 * Copyright (c) 2023 SvOlli
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FAKE_EEPROM_H
#define FAKE_EEPROM_H FAKE_EEPROM_H

uint32_t flash_size_detect();
void settings_write( void* memory, uint32_t size );
void settings_read( void* memory, uint32_t size );

#endif
