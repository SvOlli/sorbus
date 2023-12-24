#!/bin/sh

if [ $# -lt 3 ]; then
   echo >&2 "usage: $0 <bindir> <srcdir> <cpm65dir>"
   exit 1
fi

BINDIR="${1}"
shift
SRCDIR="${1}"
shift
CPM65_DIR="${1}"
shift

# hack for filtering out end of text file: character is ^Z (0x1a)
readonly EOF=""

# a hack for SvOlli's build environment, to be removed soon
if [ -d "${CPM65_DIR}/cpm65" ]; then
   CPM65_DIR="${CPM65_DIR}/cpm65"
fi

CPMEMU="${CPM65_DIR}/bin/cpmemu"
CPMASM="${CPM65_DIR}/.obj/apps+asm/apps+asm"

readonly BINDIR SRCDIR CPM65_DIR

cp "${CPM65_DIR}/apps"/*.inc "${SRCDIR}"

cd "${SRCDIR}"

for i in *.asm ; do
   file="${i%.asm}"
   echo "cpmemu asm.com a:${i} b:${i%.asm}.com"
   "${CPMEMU}" "${CPMASM}" -pA="${SRCDIR}" -pB="${BINDIR}" \
      a:${file}.asm b:${file}.com
   cat "${BINDIR}/${file}.sym" | while read line; do
   case "${line}" in
   ${EOF}*) break;;
   *) echo "${line}";;
   esac
   done >"${SRCDIR}/${file}.sym"
   rm -f "${BINDIR}/${file}.sym"
done
