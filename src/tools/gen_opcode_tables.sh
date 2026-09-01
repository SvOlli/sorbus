#!/bin/sh

set -e

readonly variants="6502 65c02 65816 65ce02 65sc02"
readonly scriptname="$(basename "${0}")"

cd "$(dirname "${0}")/../.."


mklines()
{
   local ifs="${IFS}"
   local line=0
   echo "   /* OPCODE( name, mode, reserved, bytes, cycles, extra, jump, mxe ) */"
   IFS=';'
   while read byte name mode reserved bytes cycles extra mx jump rest; do
      bitsuffix="0"

      case "${name}" in
      *[0-7]) name="${name%?}"; bitsuffix="1";;
      "") name="___";
      esac

      case "${mode}" in
      "ABS")      emode="ABS";;
      "[ABS]")    emode="ABSIL";;
      "ABSL")     emode="ABSL";;
      "ABSL,X")   emode="ABSLX";;
      "ABSL,Y")   emode="ABSLY";;
      "ABS,X")    emode="ABSX";;
      "ABS,Y")    emode="ABSY";;
      "ABS,Z")    emode="ABSZ";;
      "(ABS)")    emode="AI";;
      "(ABS,X)")  emode="AIX";;
      "")         emode="IMP";;
      "#IM")      emode="IMM";;
      "#IM,#IM")  emode="IMM2";;
      "#IML")     emode="IMML";;
      "REL")      emode="REL";;
      "RELL")     emode="RELL";;
      "(REL,S),Y") emode="RELSY";;
      "ZP") if [ "${bitsuffix}" -eq 0 ]; then
                  emode="ZP"
            else
                  emode="ZPN"
            fi;;
      "(ZP)")     emode="ZPI";;
      "(ZP,X)")   emode="ZPIX";;
      "(ZP),Y")   emode="ZPIY";;
      "(ZP),Z")   emode="ZPIZ";;
      "[ZP]")     emode="ZPIL";;
      "[ZP],Y")   emode="ZPILY";;
      "(ZP,S),Y") emode="ZPISY";;
      "ZP,REL")   emode="ZPNR";;
      "ZP,S")     emode="ZPS";;
      "ZP,X")     emode="ZPX";;
      "ZP,Y")     emode="ZPY";;
      *) echo >&2 "unknown mode: '${mode}'"; false;;
      esac

      reserved=$((reserved+0)) # make sure that it's a number
      bytes=$((bytes+0))
      cycles=$((cycles+0))
      extra=$((extra+0))
      mx=$((mx+0))
      jump=$((jump+0))
      echo -n "   /* ${byte} */ OPCODE( ${name}, ${emode}, ${reserved}, ${bytes}, ${cycles}, ${extra}, ${mx}, ${jump} )"

      if [ "${line}" -lt 255 ]; then
         echo ","
      else
         echo
      fi
      line="$((line+1))"
   done
   IFS="${ifs}"
}


mkgenerated_h()
{
   {
      cat <<__END_OF_TEXT__
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
__END_OF_TEXT__

      for i in ${variants};do
         cat "doc/opcodes${i}.csv"
      done |
         grep '^\$' |
         cut -f2 -d\; |
         sed -e 's/^\(...\)/\1, /' -e 's/\(...\)[0-7]/\1/' |
         sort -u |
         tr -d '\n' |
         fold -s -w 70 |
         sed -e 's/^/   /' -e 's/  *$//g'

      cat <<__END_OF_TEXT__
DA_MNEMONIC_END
} da_mnemonic_t;

typedef enum {
   ADDRMODE_UNDEF = 0,
   ABS,   // OPC \$1234
   ABSIL, // OPC [\$1234]
   ABSL,  // OPC \$123456
   ABSLX, // OPC \$123456,X
   ABSLY, // OPC \$123456,Y
   ABSX,  // OPC \$1234,X
   ABSY,  // OPC \$1234,Y
   ABSZ,  // OPC \$1234,Z
   AI,    // OPC (\$1234)
   AIX,   // OPC (\$1234,X)
   IMP,   // OPC
   IMM,   // OPC #\$01
   IMM2,  // OPC #\$01,#\$02
   IMML,  // OPC #\$1234
   REL,   // OPC LABEL
   RELL,  // OPC LABEL
   RELSY, // OPC (LABEL,S),Y
   ZP,    // OPC \$12
   ZPI,   // OPC (\$12)
   ZPIL,  // OPC [\$12]
   ZPILY, // OPC [\$12],Y
   ZPISY, // OPC (\$12,S),Y
   ZPN,   // OPC# \$12
   ZPNR,  // OPC# \$12,LABEL
   ZPS,   // OPC \$12,S
   ZPX,   // OPC \$12,X
   ZPY,   // OPC \$12,Y
   ZPIX,  // OPC (\$12,X)
   ZPIY,  // OPC (\$12),Y
   ZPIZ,  // OPC (\$12),Z
   ADDRMODE_END
} da_addrmode_t;

#endif
__END_OF_TEXT__
   # TODO: can da_addrtype_t be generated?
   } > "src/rp2040/disassemble/da_generated.h"
}


mkgenerated_c()
{
   {
      cat <<__END_OF_TEXT__
#include "da_generated.h"

/* automatically generated using ${scriptname} */

#define OPCODE(mn, am, reserved, bytes, cycles, extra, jump, mxe) \\
   (uint32_t)mn | (uint32_t)am << 8 | (uint32_t)reserved << 14 | \\
   (uint32_t)bytes << 15 | (uint32_t)cycles << 18 | \\
   (uint32_t)extra << 22 | (uint32_t)jump << 24 | (uint32_t)mxe << 25

/* needs to be aligned with mnemonic_t */
const char *da_mnemonics[] = {
__END_OF_TEXT__
      for i in ${variants};do
         cat "doc/opcodes${i}.csv"
      done |
         cut -f2 -d\; |
         sed -e 's/Name/???/' -e 's/^\(...\)/"\1", /' -e 's/\(...\)[0-7]/\1/' |
         sort -u |
         tr -d '\n' |
         fold -s -w 70 |
         sed -e 's/^/   /' -e 's/  *$//g'

      cat <<__END_OF_TEXT__
0
};

__END_OF_TEXT__

      for i in ${variants};do
         local infile="doc/opcodes${i}.csv"
         echo "/* automatically generated using ${infile} */"
         echo "const uint32_t da_opcodes${i}[0x100] = {"
         grep '^\$' "${infile}" | sort | mklines
         echo "};\n"
      done
   } > "src/rp2040/disassemble/da_generated.c"
}


# autoupdate 65SC02 from 65C02 data
# check if there is a difference ignoring 65C02 "enhancements" over 65SC02
md5_65c02="$(grep -v -e '^$.[7F]' -e '^$[CD]B' doc/opcodes65c02.csv | md5sum -)"
md5_65sc02="$(grep -v -e '^$.[7F]' -e '^$[CD]B' doc/opcodes65sc02.csv | md5sum -)"
if [ "${md5_65c02}" != "${md5_65sc02}" ]; then
   # first regex removes bit (re)set and test opcodes
   # second regex removes STP and WAI opcodes
   # otherwise 65C02 and 65SC02 opcodes should be the same
   sed -e 's/^\($.[7F]\);...[0-7];ZP.*$/\1;NOP;;1;1;1;;;/g' \
       -e 's/^\($[CD]B\);.*/\1;NOP;;1;1;1;;;/g' \
      <doc/opcodes65c02.csv >doc/opcodes65sc02.csv
fi

mkgenerated_h
mkgenerated_c
