#!/bin/sh

cd "$(dirname "${0}")/../.."

readonly WORKDIR="external-build/cpm65"
readonly URL="https://github.com/davidgiven/cpm65.git"
readonly BASEDIR="${PWD}"
readonly BUILDTARGET="sorbus.zip"
readonly TARGETDIR="${BASEDIR}/src/bin/cpm"

set -e

rm -rf "${WORKDIR}"
mkdir -p "${WORKDIR}"
cd "${WORKDIR}"
git clone --depth 1 "${URL}" "${PWD}"
make "${BUILDTARGET}"
readonly ZIPDIR="${PWD}"
cd "${TARGETDIR}"
7z x -y "${ZIPDIR}/${BUILDTARGET}" CPMFS CPM BDOS
cpmcp -f sorbus CPMFS '0:*.*' 0/
rm -f CPMFS

echo "Success!"
