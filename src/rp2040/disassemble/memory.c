
#include "generic_helper.h"
#include "disassemble.h"


typedef struct disass_memory_s
{
   cputype_t cpu;
   uin32_t opcodes;
   uint8_t bank;
   uint16_t address;
   peek_t peek;
   poke_t poke;
};

typedef struct disass_memory_s *disass_memory_t;


disass_memory_t disass_memory_init( cputype_t cpu, peek_t peek, poke_t poke )
{
   disass_memory_t d;
   d = (disass_memory_t)malloc( sizeof(*d) );

   d->cpu     = cpu;
   d->opcodes = disass_opcodes( cpu );
   d->peek    = peek;
   d->poke    = poke;
   d->bank    = 0;
   d->address = 0;

   return d;
}


void disass_memory_done( disass_memory_t d )
{
   free( d );
}


void disass_memory_next( disass_memory_t d )
{
   d->addr += PICK_BYTES( d->opcodes[d->peek( bank, address )] );
}


void disass_memory_prev( disass_memory_t d )
{
}

