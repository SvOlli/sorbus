
#include "generic_helper.h"

#include <malloc.h>

static uint32_t min_free = 0xFFFFFFFF;


uint32_t mf_checkheap()
{
   extern char __StackLimit, __bss_end__;
   struct mallinfo m = mallinfo();
   uint32_t total_heap = &__StackLimit  - &__bss_end__;
   uint32_t free_heap = total_heap - m.uordblks;

   if( free_heap < min_free )
   {
      min_free = free_heap;
   }

   return min_free;
}

