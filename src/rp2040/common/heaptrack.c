
#include "generic_helper.h"

#include <malloc.h>
#include <stdio.h>


static uint32_t _min_free = 0xFFFFFFFF; // least minimum possible


uint32_t ht_freemem()
{
#if PICO_ON_DEVICE
   extern char __StackLimit, __bss_end__;
   struct mallinfo m = mallinfo();
   uint32_t total_heap = &__StackLimit - &__bss_end__;
   return total_heap - m.uordblks;
#else
   return 128*1024; // on host, always assume 128k free
#endif
}


uint32_t ht_freemin()
{
   uint32_t free_heap = ht_freemem();

   if( free_heap < _min_free )
   {
      _min_free = free_heap;
   }

   return _min_free;
}


void *ht_malloc( size_t size )
{
   void *retval = malloc( size );
   (void)ht_freemin();
   return retval;
}


void ht_free( void *ptr )
{
   (void)ht_freemin();
   free( ptr );
}


void *ht_calloc( size_t nmemb, size_t size )
{
   void *retval = calloc( nmemb, size );
   (void)ht_freemin();
   return retval;
}


void *ht_realloc( void *ptr, size_t size )
{
   void *retval = realloc( ptr, size );
   (void)ht_freemin();
   return retval;
}


void ht_info( char *buffer, size_t size )
{
#if PICO_ON_DEVICE
   extern char __StackLimit, __bss_end__;
   struct mallinfo m = mallinfo();
   uint32_t total_heap = &__StackLimit - &__bss_end__;
   uint32_t free_heap = total_heap - m.uordblks;
#else
   uint32_t total_heap = 256*1024; // on host, always assume 256k available
   uint32_t free_heap = 128*1024; // on host, always assume 128k free
#endif

   snprintf( buffer, size,
             "heap total: %6u\n"
             "      free: %6u\n"
             "   minimum: %6u"
             , total_heap, free_heap, ht_freemin() );
}
