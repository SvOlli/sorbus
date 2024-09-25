#!/bin/sh

readonly inputlib="supervision.lib"
readonly outputlib="sorbus.lib"

outputdir="${1}"
if [ -z "${outputdir}" ]; then
   echo "usage: $0 <outputdir>"
   exit 1
fi
case "${outputdir}" in
/*) ;;
*) outputdir="${PWD}/${outputdir}";;
esac
readonly outputdir

set -ex

CC65_BASE_DIR="$(dirname "$(cl65 --print-target-path)")"

cd "$(dirname "${0}")"

readonly inputdir="${PWD}"

cp "${CC65_BASE_DIR}/lib/${inputlib}" "${outputdir}/${outputlib}"

for s in *.s; do
   o="${s%.s}.o"
   ca65 --cpu 65c02 -o "${outputdir}/${o}" "${inputdir}/${s}"
   cd "${outputdir}"
   ar65 a "${outputlib}" "${o}"
   cd "${inputdir}"
done

cc65 -t none -O --cpu 65c02 iotest.c
ca65 --cpu 65c02 iotest.s
ld65 -o iotest.sx4 -C sorbus.cfg iotest.o ${outputdir}/${outputlib}
rm -f iotest.o

