
#include "menu.h"

#include <stdbool.h>
#include <stdio.h>

typedef void (*menu_function_t)(void);

typedef struct native_menu_s {
   uint32_t             key_code;
   char                 *text;
   menu_function_t      function;
   struct native_menu_s *menu;
} native_menu_t;




native_menu_t menu_start[] = {
   { 'r', "reboot + exit", 0/*menu_reboot*/, 0 },
   { 'c', "continue", 0, 0 },
   { 0, 0, 0, 0 }
};


native_menu_t *menu_handle( native_menu_t *menu )
{
   int i;
   for( i = 0; menu[i].key_code; ++i )
   {
      printf( "%c: %s\n", menu[i].key_code, menu[i].text );
   }
   
   return 0;
}


void menu_run()
{
   int i;
   native_menu_t *menu = menu_start;
   while( menu )
   {
      menu = menu_handle( menu );
   }
}

