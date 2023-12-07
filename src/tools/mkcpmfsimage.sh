#!/bin/sh

set -e

if [ ${#} -ne 2 ]; then
   cat <<EOF
usage: $0 <output_image> <path_to_cpm_data>
EOF
   exit 1
fi

touch "${1}"
readonly IMAGE="$(readlink -f "${1}")"
readonly CPM_DIR="${2}"
readonly FORMAT='sorbus'
readonly BOOTIMAGE='cpm_rom.bin'

cd "${CPM_DIR}" ||
{
   echo "could not cd to '${CPM_DIR}'"
   exit 10
}

if [ ! -f diskdefs ]; then
   echo "Missing 'diskdefs' file. Are you in the right directory?"
   exit 11
fi

cpmtools_missing=0
for i in mkfs.cpm cpmcp cpmchattr; do
   type $i || {
      echo "'$i' not found. Are cpmtools installed?"
      cpmtools_missing=1
   }
done
if [ ${cpmtools_missing} -ne 0 ]; then
   echo "missing at least one cpmtool"
   exit 12
fi

rm -f "${IMAGE}"
mkfs.cpm -f "${FORMAT}" -b "${BOOTIMAGE}" "${IMAGE}"
for i in [0-9]*/*;do
   # skip directories
   [ -d "${i}" ] && continue
   user="${i%/*}"
   cpmcp -f "${FORMAT}" "${IMAGE}" "${i}" ${user}:
done
cpmchattr -f "${FORMAT}" "${IMAGE}" sr 0:ccp.sys
set +x
echo "Imagefile:"
ls -l "${IMAGE}"
echo "Contents:"
cpmls -f "${FORMAT}" -d "${IMAGE}"
echo
