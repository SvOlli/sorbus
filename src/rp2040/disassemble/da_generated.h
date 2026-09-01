#ifndef DA_GENERATED_H
#define DA_GENERATED_H DA_GENERATED_H

#include <stdint.h>

extern const uint32_t da_opcodes6502[0x100];
extern const uint32_t da_opcodes65c02[0x100];
extern const uint32_t da_opcodes65sc02[0x100];
extern const uint32_t da_opcodes65ce02[0x100];
extern const uint32_t da_opcodes65816[0x100];

extern const char *da_mnemonics[];

typedef enum {
   DA_MNEMONIC_UNDEF = 0,
   ADC, AHX, ALR, ANC, AND, ARR, ASL, ASR, ASW, AUG, AXS, BBR, BBS, BCC,
   BCS, BEQ, BIT, BMI, BNE, BPL, BRA, BRK, BRL, BSR, BVC, BVS, CLC, CLD,
   CLE, CLI, CLV, CMP, COP, CPX, CPY, CPZ, DCP, DEC, DEW, DEX, DEY, DEZ,
   EOR, INC, INW, INX, INY, INZ, ISC, JML, JMP, JSL, JSR, KIL, LAS, LAX,
   LDA, LDX, LDY, LDZ, LSR, LXA, MVN, MVP, NEG, NOP, ORA, PEA, PEI, PER,
   PHA, PHB, PHD, PHK, PHP, PHW, PHX, PHY, PHZ, PLA, PLB, PLD, PLP, PLX,
   PLY, PLZ, RAA, REP, RLA, RMB, ROL, ROR, ROW, RRA, RTI, RTL, RTN, RTS,
   SAX, SBC, SEC, SED, SEE, SEI, SEP, SHX, SHY, SLO, SMB, SRE, STA, STP,
   STX, STY, STZ, TAB, TAS, TAX, TAY, TAZ, TBA, TCD, TCS, TDC, TRB, TSB,
   TSC, TSX, TSY, TXA, TXS, TXY, TYA, TYS, TYX, TZA, WAI, WDM, XAA, XBA,
   XCE,DA_MNEMONIC_END
} da_mnemonic_t;

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
} da_addrmode_t;

#endif
