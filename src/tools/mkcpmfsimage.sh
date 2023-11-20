#!/bin/sh

set -ex

cd "$(dirname "${0}")/../bin"

readonly MKFTL='../../build/tools/mkftl.exe'
readonly FORMAT='sorbus'
readonly IMG='sorbus.img'
readonly FTL='cpm65.ftl'
readonly SECTORSIZE=512
readonly FLASHSIZE=$((12*1024))
readonly ERASESIZE=4096

if [ ! -f diskdefs ]; then
   echo "Missing 'diskdefs' file. Are you in the right directory?"
   exit 10
fi

if [ ! -x "${MKFTL}" ]; then
   make -C ../.. || exit 11
fi

cpmtools_missing=0
for i in mkfs.cpm cpmcp; do
   type $i || {
      echo "'$i' not found. Are cpmtools installed?"
      cpmtools_missing=1
   }
done
if [ ${cpmtools_missing} -ne 0 ]; then
   echo "missing at least one cpmtool"
   exit 12
fi

if [ ! -d "cpm" ]; then
   echo "missing 'cpm' directory."
   exit 13
fi

mkfs.cpm -f "${FORMAT}" "${IMG}"
for i in cpm/* cpm/[1-9]*/*;do
   # skip directories
   [ -d "${i}" ] && continue
   user=0
   case "${i}" in
   cpm/[1-9]*/*) user="$(echo ${i} | cut -f2 -d/)";;
   esac
   cpmcp -f "${FORMAT}" "${IMG}" "${i}" ${user}:
done
cpmls -f "${FORMAT}" "${IMG}"
ls -l "${IMG}"
"${MKFTL}" -p ${SECTORSIZE} -s ${FLASHSIZE} -e ${ERASESIZE} "${IMG}" -o "${FTL}"
set +x
echo "all done, flash image with:"
echo "picotool load -f -o 0x10400000 -t bin '$(readlink -f "${FTL}")'"
