
#include "dhara_flash.h"
#include "../dhara/nand.h"
#include "../dhara/map.h"

#include <string.h>
#include <stdbool.h>

#include <hardware/flash.h>
#include <pico/multicore.h>


// taken from: https://stackoverflow.com/questions/27581671/how-to-compute-log-with-the-preprocessor
#define LOG_1(n) (((n) >= 2) ? 1 : 0)
#define LOG_2(n) (((n) >= 1<<2) ? (2 + LOG_1((n)>>2)) : LOG_1(n))
#define LOG_4(n) (((n) >= 1<<4) ? (4 + LOG_2((n)>>4)) : LOG_2(n))
#define LOG_8(n) (((n) >= 1<<8) ? (8 + LOG_4((n)>>8)) : LOG_4(n))
#define LOG(n)   (((n) >= 1<<16) ? (16 + LOG_8((n)>>16)) : LOG_8(n))

static uint8_t dhara_nand_page_buffer[PAGE_SIZE];

//static struct dhara_map dhara;
static const struct dhara_nand nand =
{
   .log2_page_size = LOG(PAGE_SIZE),
   .log2_ppb = LOG(BLOCK_SIZE) - LOG(PAGE_SIZE),
   .num_blocks = (PICO_FLASH_SIZE_BYTES - FLASH_OFFSET) / BLOCK_SIZE,
};


static uint8_t dhara_map_buffer[2*PAGE_SIZE]; // should be PAGE_SIZE, maybe 128 is not enough
static struct dhara_map dhara;


// used when writing to flash to suspend and resume system
static inline void dhara_block_system( bool stop )
{
   if( stop )
   {
      // suspend other core
      multicore_lockout_start_blocking();
   }
   else
   {
      // resume other core
      multicore_lockout_end_blocking();
   }
}


/* 
 * frontend functions: sorbus native uses these to read/write
 */

/* init */ 
int dhara_flash_init()
{
   dhara_error_t err = DHARA_E_NONE;
   int retval = 0;

   dhara_map_init(&dhara, &nand, dhara_map_buffer, GC_RATIO);
   dhara_map_resume(&dhara, &err);
   if( err != DHARA_E_NONE )
   {
      retval = -1;
   }
   return retval;
}

/* read a sector from flash
 * lba: sector to read
 * data: a sector of PAGE_SIZE bytes */
int dhara_flash_read( uint16_t lba, uint8_t *data )
{
   dhara_error_t err = DHARA_E_NONE;
   int retval = 0;

   retval = dhara_map_read( &dhara, lba, data, &err );
   if( err != DHARA_E_NONE )
   {
      retval = -1;
   }
   return retval;
}

/* write a sector from flash
 * lba: sector to write
 * data: a sector of PAGE_SIZE bytes */
int dhara_flash_write( uint16_t lba, uint8_t *data )
{
   dhara_error_t err = DHARA_E_NONE;
   int retval = 0;

   retval = dhara_map_write( &dhara, lba, data, &err );
   if( err != DHARA_E_NONE )
   {
      retval = -1;
   }
   return retval;
}


/*
 * backend functions: dhara uses these to access the flash
 */

/* Is the given block bad? */
int dhara_nand_is_bad(const struct dhara_nand *n, dhara_block_t b)
{
   // no high level api
   return 0;
}


/* Mark bad the given block (or attempt to). No return value is
 * required, because there's nothing that can be done in response.
 */
void dhara_nand_mark_bad(const struct dhara_nand *n, dhara_block_t b)
{
   // no high level api
}


/* Erase the given block. This function should return 0 on success or -1
 * on failure.
 *
 * The status reported by the chip should be checked. If an erase
 * operation fails, return -1 and set err to E_BAD_BLOCK.
 */
int dhara_nand_erase(const struct dhara_nand *n, dhara_block_t b,
                     dhara_error_t *err)
{
   dhara_block_system( true );
   flash_range_erase( FLASH_OFFSET + (b * BLOCK_SIZE), BLOCK_SIZE );
   dhara_block_system( false );

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
int dhara_nand_prog(const struct dhara_nand *n, dhara_page_t p,
                    const uint8_t *data,
                    dhara_error_t *err)
{
   dhara_block_system( true );
   flash_range_program( FLASH_OFFSET + (p*BLOCK_SIZE), data, BLOCK_SIZE );
   dhara_block_system( false );

   if( err )
   {
      *err = DHARA_E_NONE;
   }
   return 0;
}


/* Check that the given page is erased */
int dhara_nand_is_free(const struct dhara_nand *n, dhara_page_t p)
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
int dhara_nand_read(const struct dhara_nand *n, dhara_page_t p,
                    size_t offset, size_t length,
                    uint8_t *data,
                    dhara_error_t *err)
{
   memcpy( data,
           (uint8_t*)XIP_NOCACHE_NOALLOC_BASE + FLASH_OFFSET + (p*BLOCK_SIZE) + offset,
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
int dhara_nand_copy(const struct dhara_nand *n,
                    dhara_page_t src, dhara_page_t dst,
                    dhara_error_t *err)
{
   dhara_nand_read( &nand, src, 0, PAGE_SIZE, dhara_nand_page_buffer, err );
   if( *err != DHARA_E_NONE )
   {
      return -1;
   }
   return dhara_nand_prog( &nand, dst, dhara_nand_page_buffer, err );
}
