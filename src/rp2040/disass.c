
#include <disass.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*addr_type_t)( char *s, uint16_t a, uint8_t ol, uint8_t oh );

void addr_none( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
}
void addr_rel( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   int8_t rel = (int8_t)ol;
   sprintf( s, "$%04x", a + 2 + rel );
}
void addr_abs( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   a = ol + (oh << 8);
   sprintf( s, "$%04x", a );
}
void addr_absX( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   a = ol + (oh << 8);
   sprintf( s, "$%04x,X", a );
}
void addr_absY( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   a = ol + (oh << 8);
   sprintf( s, "$%04x,Y", a );
}
void addr_ind( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   a = ol + (oh << 8);
   sprintf( s, "($%04x)", a );
}
void addr_indX( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   a = ol + (oh << 8);
   sprintf( s, "($%04x,X)", a );
}
void addr_imm( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   sprintf( s, "#$%02x", ol );
}
void addr_zp( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   sprintf( s, "$%02x", ol );
}
void addr_zpX( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   sprintf( s, "$%02x,X", ol );
}
void addr_zpY( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   sprintf( s, "$%02x,Y", ol );
}
void addr_zpind( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   sprintf( s, "($%02x)", ol );
}
void addr_zpindX( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   sprintf( s, "($%02x,X)", ol );
}
void addr_zpindY( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   sprintf( s, "($%02x),Y", ol );
}
void addr_zp_rel( char *s, uint16_t a, uint8_t ol, uint8_t oh )
{
   int8_t rel = (int8_t)oh;
   sprintf( s, "$%02x,$%04x", ol, a + 3 + rel );
}

addr_type_t addr_modes[] = {
   addr_none,     // 0
   addr_rel,      // 1
   addr_abs,      // 2
   addr_absX,     // 3
   addr_absY,     // 4
   addr_ind,      // 5
   addr_indX,     // 6
   addr_imm,      // 7
   addr_zp,       // 8
   addr_zpX,      // 9
   addr_zpY,      // a
   addr_zpind,    // b
   addr_zpindX,   // c
   addr_zpindY,   // d
   addr_zp_rel    // e
};

const char mnemomics[72][4] =
{
   "NOP",
   "adc",
   "and",
   "asl",
   "bbr",
   "bbs",
   "bcc",
   "bcs",
   "beq",
   "bit",
   "bmi",
   "bne",
   "bpl",
   "bra",
   "brk",
   "bvc",
   "bvs",
   "clc",
   "cld",
   "cli",
   "clv",
   "cmp",
   "cpx",
   "cpy",
   "dec",
   "dex",
   "dey",
   "eor",
   "inc",
   "inx",
   "iny",
   "jmp",
   "jsr",
   "lda",
   "ldx",
   "ldy",
   "lsr",
   "nop",
   "ora",
   "pha",
   "php",
   "phx",
   "phy",
   "pla",
   "plp",
   "plx",
   "ply",
   "rmb",
   "rol",
   "ror",
   "rti",
   "rts",
   "sbc",
   "sec",
   "sed",
   "sei",
   "smb",
   "sta",
   "stp",
   "stx",
   "sty",
   "stz",
   "tax",
   "tay",
   "trb",
   "tsb",
   "tsx",
   "txa",
   "txs",
   "tya",
   "wai"
};

const uint32_t opcodes[0x100] = 
{
   0x27000e00, /* brk  */
   0x2600260c, /* ora ($00,x) */
   0x22000007, /* .byte $02 */
   0x11000000, /* .byte $03 */
   0x25004108, /* tsb $00 */
   0x23002608, /* ora $00 */
   0x25000308, /* asl $00 */
   0x25002f18, /* rmb0 $00 */
   0x12002800, /* php  */
   0x22002607, /* ora #$00 */
   0x12000300, /* asl a */
   0x11000000, /* .byte $0B */
   0x36004102, /* tsb LEA00 */
   0x34002602, /* ora LEA00 */
   0x36000302, /* asl LEA00 */
   0x2520041e, /* bbr0 $00,L4029 */
   0x22200c01, /* bpl L4042 */
   0x2510260d, /* ora ($00),y */
   0x2500260b, /* ora ($00) */
   0x11000000, /* .byte $13 */
   0x25004008, /* trb $00 */
   0x24002609, /* ora $00,x */
   0x26000309, /* asl $00,x */
   0x25002f18, /* rmb1 $00 */
   0x12001100, /* clc  */
   0x34102604, /* ora LEA00,y */
   0x12001c00, /* inc a */
   0x11000000, /* .byte $1B */
   0x36004002, /* trb LEA00 */
   0x34102603, /* ora LEA00,x */
   0x36100303, /* asl LEA00,x */
   0x2520041e, /* bbr1 $00,L4069 */
   0x34002002, /* jsr LEA00 */
   0x2600020c, /* and ($00,x) */
   0x22000007, /* .byte $22 */
   0x11000000, /* .byte $23 */
   0x23000908, /* bit $00 */
   0x23000208, /* and $00 */
   0x25003008, /* rol $00 */
   0x25002f18, /* rmb2 $00 */
   0x12002c00, /* plp  */
   0x22000207, /* and #$00 */
   0x12003000, /* rol a */
   0x11000000, /* .byte $2B */
   0x34000902, /* bit LEA00 */
   0x34000202, /* and LEA00 */
   0x36003002, /* rol LEA00 */
   0x2520041e, /* bbr2 $00,L40A9 */
   0x22200a01, /* bmi L40C2 */
   0x2510020d, /* and ($00),y */
   0x2500020b, /* and ($00) */
   0x11000000, /* .byte $33 */
   0x24000909, /* bit $00,x */
   0x24000209, /* and $00,x */
   0x26003009, /* rol $00,x */
   0x25002f18, /* rmb3 $00 */
   0x12003500, /* sec  */
   0x34100204, /* and LEA00,y */
   0x12001800, /* dec a */
   0x11000000, /* .byte $3B */
   0x34100903, /* bit LEA00,x */
   0x34100203, /* and LEA00,x */
   0x36103003, /* rol LEA00,x */
   0x2520041e, /* bbr3 $00,L40E9 */
   0x12003200, /* rti  */
   0x26001b0c, /* eor ($00,x) */
   0x22000007, /* .byte $42 */
   0x11000000, /* .byte $43 */
   0x23000008, /* .byte $44 */
   0x23001b08, /* eor $00 */
   0x25002408, /* lsr $00 */
   0x25002f18, /* rmb4 $00 */
   0x12002700, /* pha  */
   0x22001b07, /* eor #$00 */
   0x12002400, /* lsr a */
   0x11000000, /* .byte $4B */
   0x34001f02, /* jmp LEA00 */
   0x34001b02, /* eor LEA00 */
   0x36002402, /* lsr LEA00 */
   0x2520041e, /* bbr4 $00,L4129 */
   0x22200f01, /* bvc L4142 */
   0x25101b0d, /* eor ($00),y */
   0x25001b0b, /* eor ($00) */
   0x22000007, /* .byte $53 */
   0x24000008, /* .byte $54 */
   0x24001b09, /* eor $00,x */
   0x26002409, /* lsr $00,x */
   0x25002f18, /* rmb5 $00 */
   0x12001300, /* cli  */
   0x34101b04, /* eor LEA00,y */
   0x12002a00, /* phy  */
   0x11000000, /* .byte $5B */
   0x38000002, /* .byte $5C */
   0x34101b03, /* eor LEA00,x */
   0x36102403, /* lsr LEA00,x */
   0x2520041e, /* bbr5 $00,L4169 */
   0x12003300, /* rts  */
   0x2600010c, /* adc ($00,x) */
   0x22000007, /* .byte $62 */
   0x11000000, /* .byte $63 */
   0x23003d08, /* stz $00 */
   0x23000108, /* adc $00 */
   0x25003108, /* ror $00 */
   0x25002f18, /* rmb6 $00 */
   0x12002b00, /* pla  */
   0x22000107, /* adc #$00 */
   0x12003100, /* ror a */
   0x11000000, /* .byte $6B */
   0x36001f05, /* jmp (LEA00) */
   0x34000102, /* adc LEA00 */
   0x36003102, /* ror LEA00 */
   0x2520041e, /* bbr6 $00,L41A9 */
   0x22201001, /* bvs L41C2 */
   0x2510010d, /* adc ($00),y */
   0x2500010b, /* adc ($00) */
   0x11000000, /* .byte $73 */
   0x24003d09, /* stz $00,x */
   0x24000109, /* adc $00,x */
   0x26003109, /* ror $00,x */
   0x25002f18, /* rmb7 $00 */
   0x12003700, /* sei  */
   0x34100104, /* adc LEA00,y */
   0x12002e00, /* ply  */
   0x11000000, /* .byte $7B */
   0x36001f06, /* jmp (LEA00,x) */
   0x34100103, /* adc LEA00,x */
   0x36103103, /* ror LEA00,x */
   0x2520041e, /* bbr7 $00,L41E9 */
   0x22200d01, /* bra L4202 */
   0x2600390c, /* sta ($00,x) */
   0x22000007, /* .byte $82 */
   0x11000000, /* .byte $83 */
   0x23003c08, /* sty $00 */
   0x23003908, /* sta $00 */
   0x23003b08, /* stx $00 */
   0x25003818, /* smb0 $00 */
   0x12001a00, /* dey  */
   0x22000907, /* bit #$00 */
   0x12004300, /* txa  */
   0x11000000, /* .byte $8B */
   0x34003c02, /* sty LEA00 */
   0x34003902, /* sta LEA00 */
   0x34003b02, /* stx LEA00 */
   0x2520051e, /* bbs0 $00,L4229 */
   0x22200601, /* bcc L4242 */
   0x2510390d, /* sta ($00),y */
   0x2500390b, /* sta ($00) */
   0x11000000, /* .byte $93 */
   0x24003c09, /* sty $00,x */
   0x24003909, /* sta $00,x */
   0x24003b0a, /* stx $00,y */
   0x25003818, /* smb1 $00 */
   0x12004500, /* tya  */
   0x34103904, /* sta LEA00,y */
   0x12004400, /* txs  */
   0x11000000, /* .byte $9B */
   0x34003d02, /* stz LEA00 */
   0x34103903, /* sta LEA00,x */
   0x34103d03, /* stz LEA00,x */
   0x2520051e, /* bbs1 $00,L4269 */
   0x22002307, /* ldy #$00 */
   0x2600210c, /* lda ($00,x) */
   0x22002207, /* ldx #$00 */
   0x11000000, /* .byte $A3 */
   0x23002308, /* ldy $00 */
   0x23002108, /* lda $00 */
   0x23002208, /* ldx $00 */
   0x25003818, /* smb2 $00 */
   0x12003f00, /* tay  */
   0x22002107, /* lda #$00 */
   0x12003e00, /* tax  */
   0x11000000, /* .byte $AB */
   0x34002302, /* ldy LEA00 */
   0x34002102, /* lda LEA00 */
   0x34002202, /* ldx LEA00 */
   0x2520051e, /* bbs2 $00,L42A9 */
   0x22200701, /* bcs L42C2 */
   0x2510210d, /* lda ($00),y */
   0x2500210b, /* lda ($00) */
   0x11000000, /* .byte $B3 */
   0x24002309, /* ldy $00,x */
   0x24002109, /* lda $00,x */
   0x2400220a, /* ldx $00,y */
   0x25003818, /* smb3 $00 */
   0x12001400, /* clv  */
   0x34102104, /* lda LEA00,y */
   0x12004200, /* tsx  */
   0x11000000, /* .byte $BB */
   0x34102303, /* ldy LEA00,x */
   0x34102103, /* lda LEA00,x */
   0x34102204, /* ldx LEA00,y */
   0x2520051e, /* bbs3 $00,L42E9 */
   0x22001707, /* cpy #$00 */
   0x2600150c, /* cmp ($00,x) */
   0x22000007, /* .byte $C2 */
   0x11000000, /* .byte $C3 */
   0x23001708, /* cpy $00 */
   0x23001508, /* cmp $00 */
   0x25001808, /* dec $00 */
   0x25003818, /* smb4 $00 */
   0x12001e00, /* iny  */
   0x22001507, /* cmp #$00 */
   0x12001900, /* dex  */
   0x12004600, /* wai  */
   0x34001702, /* cpy LEA00 */
   0x34001502, /* cmp LEA00 */
   0x36001802, /* dec LEA00 */
   0x2520051e, /* bbs4 $00,L4329 */
   0x22200b01, /* bne L4342 */
   0x2510150d, /* cmp ($00),y */
   0x2500150b, /* cmp ($00) */
   0x11000000, /* .byte $D3 */
   0x24000008, /* .byte $D4 */
   0x24001509, /* cmp $00,x */
   0x26001809, /* dec $00,x */
   0x25003818, /* smb5 $00 */
   0x12001200, /* cld  */
   0x34101504, /* cmp LEA00,y */
   0x12002900, /* phx  */
   0x12003a00, /* stp  */
   0x34000002, /* .byte $DC */
   0x34101503, /* cmp LEA00,x */
   0x36101803, /* dec LEA00,x */
   0x2520051e, /* bbs5 $00,L4369 */
   0x22001607, /* cpx #$00 */
   0x2600340c, /* sbc ($00,x) */
   0x22000007, /* .byte $E2 */
   0x11000000, /* .byte $E3 */
   0x23001608, /* cpx $00 */
   0x23003408, /* sbc $00 */
   0x25001c08, /* inc $00 */
   0x25003818, /* smb6 $00 */
   0x12001d00, /* inx  */
   0x22003407, /* sbc #$00 */
   0x12002500, /* nop  */
   0x11000000, /* .byte $EB */
   0x34001602, /* cpx LEA00 */
   0x34003402, /* sbc LEA00 */
   0x36001c02, /* inc LEA00 */
   0x2520051e, /* bbs6 $00,L43A9 */
   0x22200801, /* beq L43C2 */
   0x2510340d, /* sbc ($00),y */
   0x2500340b, /* sbc ($00) */
   0x11000000, /* .byte $F3 */
   0x24000008, /* .byte $F4 */
   0x24003409, /* sbc $00,x */
   0x26001c09, /* inc $00,x */
   0x25003818, /* smb7 $00 */
   0x12003600, /* sed  */
   0x34103404, /* sbc LEA00,y */
   0x12002d00, /* plx  */
   0x11000000, /* .byte $FB */
   0x34000002, /* .byte $FC */
   0x34103403, /* sbc LEA00,x */
   0x36101c03, /* inc LEA00,x */
   0x2520051e, /* bbs7 $00,L43E9 */
};

const char *dis65c02( uint16_t addr, uint8_t opcode,
                      uint8_t operand, uint8_t operand2, uint32_t *info )
{
   static char buffer[16];
   uint32_t opcode_info = opcodes[opcode];
   sprintf( &buffer[0], "%s%c ", 
            mnemomics[(opcode_info >> 8) & 0xFF],
            (opcode_info & 0x10) ? ((opcode >> 4) & 7) | '0' : ' '
            );
   
   addr_modes[opcode_info & 0xF]( &buffer[5], 0x1000, operand, operand2 );

   if( info )
   {
      *info = opcode_info;
   }

   return &buffer[0];
}
