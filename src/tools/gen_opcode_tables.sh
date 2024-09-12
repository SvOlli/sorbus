#!/bin/sh

set -e

cd "$(dirname "${0}")/../.."

mklines()
{
   local ifs="${IFS}"
   local line=0
   IFS=';'
   while read byte name mode res bytes cycles rest; do
      bitsuffix="0"
      case "${name}" in
      *[0-7]) name="${name%?}"; bitsuffix="1";;
      "") name="___";
      esac
      case "${mode}" in
      "ABS") emode="ABS";;
      "[ABS]") emode="ABSIL";;
      "?") emode="ABSL";;
      "?") emode="ABSLX";;
      "?") emode="ABSLY";;
      "ABS,X") emode="ABSX";;
      "ABS,Y") emode="ABSY";;
      "ABS,Z") emode="ABSZ";;
      "(ABS)") emode="AI";;
      "?") emode="AIL";;
      "(ABS,X)") emode="AIX";;
      "") emode="IMP";;
      "#IM") emode="IMM";;
      "#IM,#IM") emode="IMM2";;
      "#IML") emode="IMML";;
      "REL") emode="REL";;
      "?") emode="REL2";;
      "ZP") if [ "${bitsuffix}" -eq 0 ]; then
               emode="ZP"
            else
               emode="ZPN"
            fi;;
      "(ZP)") emode="ZPI";;
      "(ZP,X)") emode="ZPIX";;
      "(ZP),Y") emode="ZPIY";;
      "(ZP),Z") emode="ZPIZ";;
      "[ZP]") emode="ZPIL";;
      "[ZP],Y") emode="ZPILY";;
      "(ZP,S),Y") emode="ZPISY";;
      "ZP,REL") emode="ZPNR";;
      "ZP,S") emode="ZPS";;
      "ZP,X") emode="ZPX";;
      "ZP,Y") emode="ZPY";;
      *) echo >&2 "unknown mode: '${mode}'"; false;;
      esac
      echo -n "   /* ${byte} */ OPCODE( \"${name}\", ${emode} )"
      if [ "${line}" -lt 255 ]; then
         echo ","
      else
         echo
      fi
      line="$((line+1))"
   done
   IFS="${ifs}"
}

mkheader()
{
   local cpu="${1}"
   local infile="${2}"
   local outfile="${3}"
   
   rm -f "${outfile}"
   echo "/* automatically generated using $(basename "${0}") on ${infile} */" > "${outfile}"
   grep '^\$' "${infile}" | sort | mklines >> "${outfile}" 
}

for i in 65c02;do
   mkheader ${i} doc/opcodes${i}.csv src/rp2040/opcodes${i}.tab
done
