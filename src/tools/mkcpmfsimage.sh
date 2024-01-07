#!/bin/sh

set -e

if [ ${#} -lt 3 ]; then
   cat <<EOF
usage: $0 <output_image> <path_to_cpm_bootblock> <path_to_cpm_data> (<extra_files>...)

<path_to_cpm_data> will be scanned for directories with the numbers
"0" to "15". The files in that directory will be copied to the
corresponding "user partition".

<extra_files> are in the format "<user>.<filename>.<ext>".

got: ${0} ${@}
EOF
   exit 1
fi

readonly FORMAT='sorbus'
touch "${1}"
readonly OUTPUT="$(readlink -f "${1}")"
shift # strip off first argument: output image
readonly BOOTBLOCK="$(readlink -f "${1}")"
shift
readonly CPM_DIR="${1}"
shift # strip off second argument: bootblock

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

rm -f "${OUTPUT}"
mkfs.cpm -f "${FORMAT}" -b "${BOOTBLOCK}" "${OUTPUT}"
for i in [0-9]*/* $@;do
   # skip directories
   [ -d "${i}" ] && continue
   case "${i}" in
   /*) # argument
      basename="${i##*/}"
      user="${basename%%.*}"
      outfile="${basename#*.}"
   ;;
   */*) # src/bin/cpm/...
      user="${i%/*}"
      outfile="" # keep original filename
   ;;
   *)
      echo "don't know how to handle '${i}'"
      exit 20
   ;;
   esac
   echo cpmcp -f "${FORMAT}" "${OUTPUT}" "${i}" "${user}:${outfile}"
   cpmcp -f "${FORMAT}" "${OUTPUT}" "${i}" "${user}:${outfile}"
done
cpmchattr -f "${FORMAT}" "${OUTPUT}" sr 0:ccp.sys
set +x
echo "Imagefile:"
ls -l "${OUTPUT}"
echo "Contents:"
cpmls -f "${FORMAT}" -d "${OUTPUT}"
echo
