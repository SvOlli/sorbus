#ifndef _NATIVE_MENU_H_
#define _NATIVE_MENU_H_ _NATIVE_MENU_H_

#include <stdint.h>

void menu_run();
void print_welcome();

/* Helpers for handling commandline input */
const char *get_hex( const char *input, uint32_t *value, uint32_t digits );
const char *get_dec( const char *input, uint32_t *value );
const char *skip_space( const char *input );


#endif