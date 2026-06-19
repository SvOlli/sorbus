/**
 * Copyright (c) 2026 Sven Oliver "SvOlli" Moll
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This is part of a disassembler for the Sorbus Computer
 */

#ifndef __DA_BASE_H__
#define __DA_BASE_H__ __DA_BASE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "base_types.h"
#include "da_generated.h"

// maximum cycles any 6502 variant could take
#define DA_CPU_MAXCYCLES (10)

// values for the fulltrace.eval
#define DA_EVAL_MIN (0)
#define DA_EVAL_MAX (7)

/**
 * fullinfo_t is a 64 bit value wrapping whole evaluation and disassembly
 * of trace entry into one. The lower 32bits are the trace as taken from
 * GPIOs and defined in src/rp2040/common/bus.h.
 *
 * The value is layed out like this:
 * 0x7766554433221111
 * 1111: address
 * 22:   data at address
 * 33:   control lines: r/w(0), clock(1), rdy(2), irq(3), nmi(4), reset(5) (two bits spare)
 * 44:   data at address + 1 (required for single step disassembly)
 * 55:   data at address + 2 (required for single step disassembly)
 * 66:   data at address + 3 (required for single step disassembly)
 * 77:   evaluation (details to be defined: two bits for number of extra data being valid)
 *                  (details to be defined: three bits for MXE of 65816 CPU)
 *                  (details to be defined: three bits evaluation/certainty)
 *
 * NOT supported is keeping track of 65816 bank address (d0-d7 in low clock state)
 */

typedef union
{
   uint64_t       raw      :64;
   struct {
      uint32_t    trace    :30;
      uint8_t     tracex   : 2; /* buffer used for alignment: same as bits30_31 */
      uint32_t    extra    :32;
   };
   struct {
      /* lower 32bit are the same as trace from GPIOs */
      uint16_t    address  :16;
      uint8_t     data     : 8;
      bool        rw       : 1;
      bool        clock    : 1;
      bool        rdy      : 1;
      bool        irq      : 1;
      bool        nmi      : 1;
      bool        reset    : 1;
      uint8_t     bits30_31: 2; /* spare */

      /* upper 32bit contain additional data for disassembly */
      uint8_t     data1    : 8;
      uint8_t     data2    : 8;
      uint8_t     data3    : 8;
      uint8_t     dataused : 2;
      bool        m816     : 1; /* reverse meaning from CPU 'M' flag: 1=16 bit */
      bool        x816     : 1; /* reverse meaning from CPU 'X' flag: 1=16 bit */
      uint8_t     eval     : 3; /* most probably this can be reduced to ": 2" */
      bool        n816     : 1; /* reverse meaning from CPU 'E' flag: 1=native */
   };
} da_fullinfo_t;


typedef enum {
   DA_SHOW_NOTHING = 0,
   DA_SHOW_ADDRESS = 1 << 0,
   DA_SHOW_HEXDUMP = 1 << 1
} da_show_t;


typedef enum
{
   FLAG_UNKNOWN = 0,
   FLAG_UNSET   = 1,
   FLAG_SET     = 2,
   /* flags in log are inverted compared to CPU */
   FLAG_CPU1    = FLAG_UNSET,
   FLAG_CPU0    = FLAG_SET,
   FLAG_MISSING = 3
} da_cpu_flag_t;


typedef enum {
   DA_FLAG_NONE         = 0x00,
   DA_FLAG_PAD          = 0x01,
   DA_FLAG_BRK_OVERRIDE = 0x02
} da_flags_t;


/* pick misc things from opcode table */
da_mnemonic_t da_pick_mnemonic( cputype_t cpu, uint8_t opcode );
da_addrmode_t da_pick_addrmode( cputype_t cpu, uint8_t opcode );
bool da_pick_reserved( cputype_t cpu, uint8_t opcode );
uint8_t da_pick_bytes( cputype_t cpu, uint8_t opcode );
uint8_t da_pick_cycles( cputype_t cpu, uint8_t opcode );
uint8_t da_pick_extra( cputype_t cpu, uint8_t opcode );
bool da_pick_jump( cputype_t cpu, uint8_t opcode );
uint8_t da_pick_mx( cputype_t cpu, uint8_t opcode );
uint8_t da_pick_bytes816( cputype_t cpu, da_fullinfo_t fullinfo );
bool da_is_imm16_mx( cputype_t cpu, uint8_t opcode, bool m, bool x );

/* different helper functions */
/* can this opcode be a 16 bit immediate (#$xxxx)? */
bool da_is_imm16( const da_fullinfo_t fullinfo );
/* get table of opcode configuration */
const uint32_t *da_get_opcodes( cputype_t cpu );

/*
 * check if both provided da_fullinfo_t describe the same action
 * this is not a binary compare, but evaluates the data
 */
uint8_t da_fullinfo_isequal( cputype_t cpu, da_fullinfo_t fi1, da_fullinfo_t fi2 );

/*
 * const char f can be:
 * a^ (4 chars): hex address
 * c  (1 char):  base cycles
 * C  (1 char):  extra cycles
 * d^ (2 chars): hex data
 * e  (1 char):  value of eval (0..7)
 * I  (1 char):  space or 'I', if IRQ low
 * N  (1 char):  space or 'N', if NMI low
 * n  (3 chars): internal trace flags NMX, only used on 65816
 * o^ (9 chars): additional hex data as used by opcode, shows up to three
 *               bytes with leading spaces, or spaces if data is not used
 * R  (1 char):  space or 'R', if reset low
 * S  (1 char):  space or 'S', if RDY low
 * t  (1 char):  data as raw char (<0x20 inversed)
 * T  (3 chars): additional data (like o) as above
 * w  (1 char): 'r' or 'w' for read or write
 * x^ (9 chars): additional hex data as available in data, shows up to three
 *               bytes with leading spaces, or spaces if data is not used
 * y:            disassembly, if eval >= 3
 * Y:            disassembly, always
 * Z (16 chars): raw 64 bit value as hex for debugging
 *
 * characters maked with ^ will produce hex numbers in upper case,
 * if upper case letter is used
 * other characters like braces, brackets, everything <= '@' will be printed
 * as itself
 * available:   B  EFGH JKLM  PQ   UVW
 * characters:  b   fghijklm  pqrs uv   z
 */
int da_sn_fullinfo( char *b, size_t bsize, cputype_t cpu,
                    const char f, da_fullinfo_t fullinfo, da_flags_t flags );

/*
 * works the same as da_fullinfo_sn, except for a single character, it
 * now works for a null terminated string
 * also these extra chars are supported
 * E:            stop output, if eval is < 3
 * example:
 * "a w d RNIS"
 * will result in
 * "e28b r b0     " <- no RNIS
 */
int da_snf_fullinfo( char *b, size_t bsize, cputype_t cpu,
                     const char *f, da_fullinfo_t fullinfo, da_flags_t flags );

/* like snprintf, but for disassembly */
int da_sn_text( char *b, size_t bsize, cputype_t cpu,
                da_fullinfo_t fullinfo, da_flags_t flags );

/* returns text version of CPU variant */
const char *da_cputype_name( cputype_t cpu );

/* like snprintf, but for cputype name */
int da_sn_cpuname( char *b, size_t bsize, cputype_t cpu );

#endif
