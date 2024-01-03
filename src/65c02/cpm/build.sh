#!/bin/sh

set -e

if [ $# -lt 2 ]; then
   echo >&2 "usage: $0 <bindir> <cpm65dir>"
   exit 1
fi

BINDIR="${1}"
shift
CPM65_DIR="${1}"
shift
SRCDIR="$(readlink -f "$(dirname "${0}")")"

PATH="${PATH}:/opt/llvm-mos/bin"

# hack for filtering out end of text file: character is ^Z (0x1a)
readonly EOF=""

# a hack for SvOlli's build environment, to be removed soon
if [ -d "${CPM65_DIR}/cpm65" ]; then
   CPM65_DIR="${CPM65_DIR}/cpm65"
fi

CPMEMU="${CPM65_DIR}/bin/cpmemu"
CPMASM="${CPM65_DIR}/.obj/apps+asm/apps+asm"

readonly BINDIR SRCDIR CPM65_DIR

cp "${CPM65_DIR}/apps"/*.inc .

for i in "${SRCDIR}"/*.asm ; do
   [ -f "${i}" ] || continue
   file="${i##*/}"
   file="${file%.asm}"
   cp "${SRCDIR}/${file}.asm" .
   "${CPMEMU}" "${CPMASM}" -pA="${PWD}" -pB="${BINDIR}" \
      a:${file}.asm b:${file}.com
   cat "${BINDIR}/${file}.sym" | while read line; do
   case "${line}" in
   ${EOF}*) break;;
   *) echo "${line}";;
   esac
   done >"${file}.sym"
   rm -f "${BINDIR}/${file}.sym"
done

for i in "${SRCDIR}"/*.S; do
   [ -f "${i}" ] || continue
   file="${i##*/}"
   file="${file%.S}"
   mos-cpm65-clang -c -I"${CPM65_DIR}/.obj/include+include" \
      -o "${file}.o" "${SRCDIR}/${file}.S"
   mos-cpm65-clang -o "${BINDIR}/${file}.com" "${file}.o"
   rm -f "${BINDIR}/${file}.com.elf"
done

for i in "${SRCDIR}"/*.c; do
   [ -f "${i}" ] || continue
   file="${i##*/}"
   file="${file%.c}"
   mos-cpm65-clang -c -I"${CPM65_DIR}/.obj/lib+cpm65_hdrs" \
      -Os -g -Wno-main-return-type -o "${file}.o" "${SRCDIR}/${file}.c"
   mos-cpm65-clang -o "${BINDIR}/${file}.com" "${file}.o" \
      "${CPM65_DIR}/.obj/lib+cpm65/lib+cpm65.a"
   rm -f "${BINDIR}/${file}.com.elf"
done
