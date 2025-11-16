
#include "dhara_flash.h"
#include "3rdparty/dhara/error.h"
#include "3rdparty/dhara/map.h"
#include "3rdparty/dhara/nand.h"

#include <string.h>
#include <stdbool.h>

#include <hardware/flash.h>
#include <hardware/sync.h>
#include <pico/multicore.h>

#include <stdio.h>

#if (PAGE_SIZE != 0x200) || (SECTOR_SIZE != 0x80)
#warn this code has been only tested on PAGE_SIZE=512 bytes and a SECTOR_SIZE=128 bytes
#endif
#if (PAGE_SIZE & 0xFF) != 0
#error PICO only writes multiples of 256 bytes to flash
#endif

// taken from: https://stackoverflow.com/questions/27581671/how-to-compute-log-with-the-preprocessor
#define LOG_1(n) (((n) >= 2) ? 1 : 0)
#define LOG_2(n) (((n) >= 1<<2) ? (2 + LOG_1((n)>>2)) : LOG_1(n))
#define LOG_4(n) (((n) >= 1<<4) ? (4 + LOG_2((n)>>4)) : LOG_2(n))
#define LOG_8(n) (((n) >= 1<<8) ? (8 + LOG_4((n)>>8)) : LOG_4(n))
#define LOG(n)   (((n) >= 1<<16) ? (16 + LOG_8((n)>>16)) : LOG_8(n))

static uint8_t dhara_nand_page_buffer[PAGE_SIZE];
static uint8_t cache[PAGE_SIZE];
static int32_t cache_page;

#define FLASH_DRIVE_OFFSET      (FLASH_DRIVE_START-0x10000000)
#define SECTOR_MASK ((PAGE_SIZE / SECTOR_SIZE)-1)

//static struct dhara_map dhara;
static const struct dhara_nand nand =
{
   .log2_page_size = LOG(PAGE_SIZE),
   .log2_ppb = LOG(BLOCK_SIZE) - LOG(PAGE_SIZE),
   .num_blocks = (PICO_FLASH_SIZE_BYTES - FLASH_DRIVE_OFFSET) / BLOCK_SIZE,
};

static uint8_t dhara_map_buffer[PAGE_SIZE];
static struct dhara_map dhara;


/*
 * frontend functions: Sorbus JAM (Just Another Machine) uses these
 */


/* init: returns number of sectors */
uint16_t dhara_flash_init()
{
   dhara_error_t err = DHARA_E_NONE;
   int retval = 0;
   cache_page = -1;
   const uint16_t page_sector_ratio = PAGE_SIZE / SECTOR_SIZE;

   dhara_map_init( &dhara, &nand, dhara_map_buffer, GC_RATIO );
   retval = dhara_map_resume( &dhara, &err );
   if( err == DHARA_E_NONE )
   {
      retval = dhara_map_capacity( &dhara ) * page_sector_ratio;
   }
   return retval;
}


/* get some info for debugging purposes */
void dhara_flash_info( dhara_flash_info_t *info )
{
   dhara_error_t err = DHARA_E_NONE;
   char sector[SECTOR_SIZE];
   int retval = 0;
   if( !info )
   {
      return;
   }
   retval = dhara_flash_read( 0, &sector[0] );
   info->sector_size    = SECTOR_SIZE;
   info->page_size      = PAGE_SIZE;
   info->erase_size     = BLOCK_SIZE;
   info->erase_cells    = (PICO_FLASH_SIZE_BYTES-FLASH_DRIVE_OFFSET)/BLOCK_SIZE;
   info->pages          = dhara_map_capacity( &dhara );
   info->sectors        = info->pages * PAGE_SIZE / SECTOR_SIZE;
   info->gc_ratio       = GC_RATIO;
   info->read_status    = (uint32_t)retval;
   info->read_errcode   = (uint32_t)err;
}


/* read a sector from flash
 * lba: sector to read
 * data: a sector of PAGE_SIZE bytes
 * return: 0 success, -1 failure */
int dhara_flash_read( uint16_t lba, uint8_t *data )
{
   dhara_error_t err = DHARA_E_NONE;
   int retval = 0;
   uint16_t page = lba / (PAGE_SIZE/SECTOR_SIZE);
   uint offset = SECTOR_SIZE * (lba & SECTOR_MASK);

   if( cache_page != page )
   {
      retval = dhara_map_read( &dhara, page, &cache[0], &err );
      cache_page = page;
      if( err != DHARA_E_NONE )
      {
         retval = -1;
         cache_page = -1;
      }
   }
   memcpy( data, &cache[offset], SECTOR_SIZE );

   return retval;
}


/* write a sector from flash
 * lba: sector to write
 * data: a sector of PAGE_SIZE bytes
 * return: 0 success, -1 failure */
int dhara_flash_write( uint16_t lba, const uint8_t *data )
{
   dhara_error_t err = DHARA_E_NONE;
   int retval = 0;
   uint16_t page = lba / (PAGE_SIZE/SECTOR_SIZE);
   uint offset = SECTOR_SIZE * (lba & SECTOR_MASK);

   if( cache_page != page )
   {
      retval = dhara_map_read( &dhara, page, &cache[0], &err );
      cache_page = page;
      if( err != DHARA_E_NONE )
      {
         retval = -1;
         cache_page = -1;
         return retval;
      }
   }
   memcpy( &cache[offset], data, SECTOR_SIZE );
   retval = dhara_map_write( &dhara, page, &cache[0], &err );
   if( err != DHARA_E_NONE )
   {
      retval = -1;
   }

   return retval;
}


/* allow the target to "delete" a sector
 * lba: sector to discard
 * return: 0 success, -1 failure */
int dhara_flash_trim( uint16_t lba )
{
   dhara_error_t err = DHARA_E_NONE;
   int retval = 0;

   retval = dhara_map_trim( &dhara, lba, &err );
   if( err != DHARA_E_NONE )
   {
      retval = -1;
   }
   else
   {
      if( cache_page == lba )
      {
         cache_page = -1;
      }
   }

   return retval;
}


/* move data from journal to sectors */
void dhara_flash_sync()
{
   dhara_map_sync( &dhara, 0 ); // 0 = pointer to err
}


/*
 * backend functions: dhara uses these to access the flash
 */

/* Is the given block bad? */
int dhara_nand_is_bad( const struct dhara_nand *n, dhara_block_t b )
{
   // no high level Pico API
   return 0;
}


/* Mark bad the given block (or attempt to). No return value is
 * required, because there's nothing that can be done in response.
 */
void dhara_nand_mark_bad( const struct dhara_nand *n, dhara_block_t b )
{
   // no high level Pico API
}


/* Erase the given block. This function should return 0 on success or -1
 * on failure.
 *
 * The status reported by the chip should be checked. If an erase
 * operation fails, return -1 and set err to E_BAD_BLOCK.
 */
int dhara_nand_erase( const struct dhara_nand *n, dhara_block_t b,
                      dhara_error_t *err )
{
   uint32_t interrupts = save_and_disable_interrupts();
   multicore_lockout_start_blocking();
   flash_range_erase( FLASH_DRIVE_OFFSET + (b * BLOCK_SIZE), BLOCK_SIZE );
   multicore_lockout_end_blocking();
   restore_interrupts( interrupts );

   if( err )
   {
      *err = DHARA_E_NONE;
   }
   return 0;
}


/* Program the given page. The data pointer is a pointer to an entire
 * page ((1 << log2_page_size) bytes). The operation status should be
 * checked. If the operation fails, return -1 and set err to
 * E_BAD_BLOCK.
 *
 * Pages will be programmed sequentially within a block, and will not be
 * reprogrammed.
 */
int dhara_nand_prog( const struct dhara_nand *n, dhara_page_t p,
                     const uint8_t *data,
                     dhara_error_t *err )
{
   uint32_t interrupts = save_and_disable_interrupts();
   multicore_lockout_start_blocking();
   flash_range_program( FLASH_DRIVE_OFFSET + (p*PAGE_SIZE), data, PAGE_SIZE );
   multicore_lockout_end_blocking();
   restore_interrupts( interrupts );

   if( err )
   {
      *err = DHARA_E_NONE;
   }
   return 0;
}


/* Check that the given page is erased */
int dhara_nand_is_free( const struct dhara_nand *n, dhara_page_t p )
{
   dhara_error_t err = DHARA_E_NONE;

   // TODO: fix parameters
   dhara_nand_read( &nand, p, 0, PAGE_SIZE, dhara_nand_page_buffer, &err );
   if( err != DHARA_E_NONE )
   {
      return 0;
   }
   for( int i=0; i < PAGE_SIZE; i++ )
   {
      if( dhara_nand_page_buffer[i] != 0xff )
      {
         return 0;
      }
   }
   return 1;
}


/* Read a portion of a page. ECC must be handled by the NAND
 * implementation. Returns 0 on sucess or -1 if an error occurs. If an
 * uncorrectable ECC error occurs, return -1 and set err to E_ECC.
 */
int dhara_nand_read( const struct dhara_nand *n, dhara_page_t p,
                     size_t offset, size_t length,
                     uint8_t *data,
                     dhara_error_t *err )
{
   memcpy( data,
           (uint8_t*)XIP_NOCACHE_NOALLOC_BASE + FLASH_DRIVE_OFFSET + (p*PAGE_SIZE) + offset,
           length );
   if( err )
   {
      *err = DHARA_E_NONE;
   }
   return 0;
}


/* Read a page from one location and reprogram it in another location.
 * This might be done using the chip's internal buffers, but it must use
 * ECC.
 */
int dhara_nand_copy( const struct dhara_nand *n,
                     dhara_page_t src, dhara_page_t dst,
                     dhara_error_t *err )
{
   dhara_nand_read( &nand, src, 0, PAGE_SIZE, dhara_nand_page_buffer, err );
   if( *err != DHARA_E_NONE )
   {
      return -1;
   }
   return dhara_nand_prog( &nand, dst, dhara_nand_page_buffer, err );
}
