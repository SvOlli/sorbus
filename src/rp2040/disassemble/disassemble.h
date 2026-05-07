
#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H DISASSEMBLE_H

/* this header is intended to be independent from any PICO related code */

#include <stdbool.h>
#include <stdint.h>

#define DISASS_MAX_STR_LENGTH    (40)

#if PICO_ON_DEVICE

#include "bus.h"

/* also includes base_types.h */
#include "generic_helper.h"

#else
#include "base_types.h"

#define mf_malloc(s)    malloc(s)
#define mf_calloc(n,s)  calloc(n,s)
#define mf_realloc(p,s) realloc(p,s)
#define mf_free(p)      free(p)

/* alternative rp2040_purple notations for faster access (gains ~7%) */
#define BUS_CONFIG_mask_address  (0x0000FFFF)
#define BUS_CONFIG_mask_data     (0x00FF0000)
#define BUS_CONFIG_mask_rw       (0x01000000)
#define BUS_CONFIG_mask_clock    (0x02000000)
#define BUS_CONFIG_mask_rdy      (0x04000000)
#define BUS_CONFIG_mask_irq      (0x08000000)
#define BUS_CONFIG_mask_nmi      (0x10000000)
#define BUS_CONFIG_mask_reset    (0x20000000)
#define BUS_CONFIG_mask_input    (0x01FFFFFF)
#define BUS_CONFIG_mask_output   (0x3EFF0000)
#define BUS_CONFIG_shift_data    (16)
#define BUS_CONFIG_shift_address (0)

#endif


/* decide on what to display with disassembly */
typedef enum {
   DISASS_SHOW_NOTHING = 0,
   DISASS_SHOW_ADDRESS = 1 << 0,
   DISASS_SHOW_HEXDUMP = 1 << 1
} disass_show_t;

typedef enum {
   MNEMONIC_UNDEF = 0,
   ADC,
   AHX,
   ALR,
   ANC,
   AND,
   ARR,
   ASL,
   ASR,
   ASW,
   AUG,
   AXS,
   BBR,
   BBS,
   BCC,
   BCS,
   BEQ,
   BIT,
   BMI,
   BNE,
   BPL,
   BRA,
   BRK,
   BRL,
   BSR,
   BVC,
   BVS,
   CLC,
   CLD,
   CLE,
   CLI,
   CLV,
   CMP,
   COP,
   CPX,
   CPY,
   CPZ,
   DCP,
   DEC,
   DEW,
   DEX,
   DEY,
   DEZ,
   EOR,
   INC,
   INW,
   INX,
   INY,
   INZ,
   ISC,
   JML,
   JMP,
   JSL,
   JSR,
   KIL,
   LAS,
   LAX,
   LDA,
   LDX,
   LDY,
   LDZ,
   LSR,
   LXA,
   MVN,
   MVP,
   NEG,
   NOP,
   ORA,
   PEA,
   PEI,
   PER,
   PHA,
   PHB,
   PHD,
   PHK,
   PHP,
   PHW,
   PHX,
   PHY,
   PHZ,
   PLA,
   PLB,
   PLD,
   PLP,
   PLX,
   PLY,
   PLZ,
   RAA,
   REP,
   RLA,
   RMB,
   ROL,
   ROR,
   ROW,
   RRA,
   RTI,
   RTL,
   RTN,
   RTS,
   SAX,
   SBC,
   SEC,
   SED,
   SEE,
   SEI,
   SEP,
   SHX,
   SHY,
   SLO,
   SMB,
   SRE,
   STA,
   STP,
   STX,
   STY,
   STZ,
   TAB,
   TAS,
   TAX,
   TAY,
   TAZ,
   TBA,
   TCD,
   TCS,
   TDC,
   TRB,
   TSB,
   TSC,
   TSX,
   TSY,
   TXA,
   TXS,
   TXY,
   TYA,
   TYS,
   TYX,
   TZA,
   WAI,
   WDM,
   XAA,
   XBA,
   XCE,
   MNEMONIC_END
} mnemonic_t;

typedef enum {
   ADDRMODE_UNDEF = 0,
   ABS,   // OPC $1234
   ABSIL, // OPC [$1234]
   ABSL,  // OPC $123456
   ABSLX, // OPC $123456,X
   ABSLY, // OPC $123456,Y
   ABSX,  // OPC $1234,X
   ABSY,  // OPC $1234,Y
   ABSZ,  // OPC $1234,Z
   AI,    // OPC ($1234)
   AIL,   // OPC ($123456)
   AIX,   // OPC ($1234,X)
   IMP,   // OPC
   IMM,   // OPC #$01
   IMM2,  // OPC #$01,#$02
   IMML,  // OPC #$1234
   REL,   // OPC LABEL
   RELL,  // OPC LABEL
   RELSY, // OPC (LABEL,S),Y
   ZP,    // OPC $12
   ZPI,   // OPC ($12)
   ZPIL,  // OPC [$12]
   ZPILY, // OPC [$12],Y
   ZPISY, // OPC ($12,S),Y
   ZPN,   // OPC# $12
   ZPNR,  // OPC# $12,LABEL
   ZPS,   // OPC $12,S
   ZPX,   // OPC $12,X
   ZPY,   // OPC $12,Y
   ZPIX,  // OPC ($12,X)
   ZPIY,  // OPC ($12),Y
   ZPIZ,  // OPC ($12),Z
   ADDRMODE_END
} addrmode_t;

typedef enum {
   DISASS_JMP_CUSTOM = 0,
   DISASS_JMP_ABS,
   DISASS_JMP_REL,
   DISASS_JMP_RELL,
} disass_jmp_t;


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
      /* lower 32bit are the same as trace from GPIOs */
      uint16_t    address  :16;
      uint8_t     data     : 8;
      bool        rw       : 1;
      bool        clock    : 1;
      bool        rdy      : 1;
      bool        irq      : 1;
      bool        nmi      : 1;
      bool        reset    : 1;
      uint8_t     bits30_31: 2;

      /* upper 32bit contain additional data for disassembly */
      uint8_t     data1    : 8;
      uint8_t     data2    : 8;
      uint8_t     data3    : 8;
      uint8_t     dataused : 2;
      bool        m816     : 1; /* reverse meaning from CPU 'M' flag: 1=16 bit */
      bool        x816     : 1; /* reverse meaning from CPU 'X' flag: 1=16 bit */
      uint8_t     eval     : 3;
      bool        n816     : 1; /* reverse meaning from CPU 'E' flag: 1=native */
   };
} fullinfo_t;

extern const char *mnemonics[];
extern uint32_t opcodes6502[0x100];
extern uint32_t opcodes65c02[0x100];
extern uint32_t opcodes65sc02[0x100];
extern uint32_t opcodes65ce02[0x100];
extern uint32_t opcodes65816[0x100];

/* oc can also by a 32 bit sample taken from GPIO, will be truncated */
uint32_t disass_opcode( uint8_t oc );

#define PICK_MNEMONIC(o) (((o) >>  0) & 0xFF)
#define PICK_ADDRMODE(o) (((o) >>  8) & 0x3F)
#define PICK_RESERVED(o) (((o) >> 14) & 0x01)
#define PICK_BYTES(o)    (((o) >> 15) & 0x07)
#define PICK_CYCLES(o)   (((o) >> 18) & 0x0F)
#define PICK_EXTRA(o)    (((o) >> 22) & 0x03)
#define PICK_JUMP(o)     (((o) >> 24) & 0x01)
#define PICK_MX(o)       (((o) >> 25) & 0x03)

typedef enum
{
   FLAG_UNKNOWN = 0,
   FLAG_UNSET   = 1,
   FLAG_SET     = 2,
   FLAG_0       = FLAG_UNSET,
   FLAG_1       = FLAG_SET,
   FLAG_MISSING = 3
} cpu_flag_t;

struct disass_fulltrace_s
{
   cputype_t   cpu;
   uint32_t    entries;
   fullinfo_t  *fullinfo;
   uint32_t    *opcodes;
   cpu_flag_t  flag_n; // $80 negaitve
   cpu_flag_t  flag_v; // $40 overflow
                       // $20 -
   cpu_flag_t  flag_i; // $10 brk
   cpu_flag_t  flag_d; // $08 enable bcd mode
   cpu_flag_t  flag_b; // $04 irq disable
   cpu_flag_t  flag_z; // $02 zero
   cpu_flag_t  flag_c; // $01 carry
   // 65816 only below
   cpu_flag_t  flag_e; // $01 set/read with XCE
   cpu_flag_t  flag_m; // $20 native: shared with unused flag
   cpu_flag_t  flag_x; // $10 native: shared with brk
};
typedef struct disass_fulltrace_s *disass_fulltrace_t;

#define EVAL_MIN (0)
#define EVAL_MAX (7)
#define BOUNDSBUFFER (16) // 65816 can take up to 9 cycles (16bit rmw)

#define deceval(x) if(x > EVAL_MIN) { --x; }
#define inceval(x) if(x < EVAL_MAX) { ++x; }


uint8_t pick_mnemonic( disass_fulltrace_t d, int pos );
uint8_t pick_addrmode( disass_fulltrace_t d, int pos );
uint8_t pick_reserved( disass_fulltrace_t d, int pos );
uint8_t pick_bytes( disass_fulltrace_t d, int pos );
uint8_t pick_cycles( disass_fulltrace_t d, int pos );
uint8_t pick_extra( disass_fulltrace_t d, int pos );
uint8_t pick_jump( disass_fulltrace_t d, int pos );
bool    pick_m( disass_fulltrace_t d, int pos );
bool    pick_x( disass_fulltrace_t d, int pos );


/*
 * convert enum to a string for display purposes
 */
const char *cputype_name( cputype_t cputype );

/*
 * somehow doing the reverse as cputype_name
 */
cputype_t getcputype( const char *argi );

/*
 * print trace data in format: aaaa r dd
 */
const char* decode_trace( uint32_t state, bool bank_enabled, uint8_t bank );

/* set cpu instruction set to disassemble */
uint32_t *disass_set_cpu( cputype_t cpu );
cputype_t disass_get_cpu();

/* for 65816, set if accumulator or index register is running in 16 bit mode */
void disass_mx816( bool m, bool x );

/* what should disassembler show? (see typedef above for details) */
void disass_show( disass_show_t show );

/* run disassembler for bytes of continuous memory */
const char *disass( uint32_t addr, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3 );
const char *disass_brk1( uint32_t addr, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3 );

/* wrapper for call above to pull everything from trace entry */
const char *disass_trace( fullinfo_t fullinfo );

/* for debugging purposes only */
uint8_t disass_bytes( uint8_t opcode );

/* get minimum clockcycles used by an opcode */
uint8_t disass_basecycles( uint8_t p0 );

/* get maximum number of extra clockcycles (e.g. for pagecrossing) */
uint8_t disass_extracycles( uint8_t p0 );

/* check if the 65(S)C02 might use extra clock cycle for BCD operations */
bool disass_bcdextracycles( uint8_t p0 );

/* get address mode */
addrmode_t disass_addrmode( uint8_t p0 );

/* check if opcode is an instruction that might jump */
bool disass_is_jump( uint8_t p0 );

/* check if trace value is writing to bus */
bool trace_is_write( uint32_t trace );

/* get address bus state from trace */
uint16_t trace_address( uint32_t trace );

/* get data bus state from trace */
uint8_t trace_data( uint32_t trace );


/* just run some qualified guesses on the trace */
void disass_historian_assumptions( disass_fulltrace_t d );


/* subroutines defined for beancounter */
/* return true if code pos is at end of buffer */
bool bc_atend( disass_fulltrace_t d, uint32_t pos );
/* compare two addresses and returns true if they are on different pages */
bool bc_pagecross( uint16_t addr0, uint16_t addr1 );
/* return number of cycles required by a vector pull or 0 if no vectorpull */
uint8_t bc_vectorpull( disass_fulltrace_t d, uint32_t pos, uint16_t addr, bool rw );
/* return number of cycles required by an interrupt or 0 if no interrupt occurred */
uint8_t bc_interrupt( disass_fulltrace_t d, uint32_t pos );
/* return number of cycles used by a branch */
uint8_t bc_jump_rel8( disass_fulltrace_t d, uint32_t pos );
/* return number of cycles used by a "jumping" instruction (includes branch) */
uint8_t bc_jump( disass_fulltrace_t d, uint32_t pos );


/* subroutines shared between beancounter16.c and beancounter.c */
uint8_t bc_interrupt_65816( disass_fulltrace_t d, uint32_t pos );
uint8_t bc_65816( disass_fulltrace_t d, uint32_t pos );
bool is_imm16( const fullinfo_t fullinfo );

uint8_t disassemble_cycles( disass_fulltrace_t d, uint32_t pos );
const char *disass_fullinfo_inbounds( disass_fulltrace_t d, uint32_t pos );


/* convert ringbuffer to fulltrace data
 * cputype:
 * trace:   raw ringbuffer
 * entries: number of entries to be converted
 * start:   start withing ringbuffer
 *
 * output contains aligned fullinfo_t with padding at start and end
 */
disass_fulltrace_t disass_fulltrace_init( cputype_t cputype,
   uint32_t *trace, uint32_t entries, uint32_t start );

/* clean up data created by disass_historian_init */
void disass_fulltrace_done( disass_fulltrace_t d );

/* convenience function to generate disassembly output for a specific entry
 * recalling function invalidates/overwrite previously returned data
 */
const char *disass_fulltrace_entry( disass_fulltrace_t d, uint32_t entry );

uint8_t disassemble_beancounter_single( disass_fulltrace_t d, uint32_t pos );
void disassemble_beancounter( disass_fulltrace_t d, uint32_t start );

/* moved to own file */
uint8_t disassemble_beancounter_single_65816( disass_fulltrace_t d, uint32_t pos );

uint8_t disass_fullinfo_isequal( uint32_t *opcodes, fullinfo_t fi1, fullinfo_t fi2 );

#endif
