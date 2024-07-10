#!/bin/sh

cd "$(dirname "${0}")/../.."

readonly WORKDIR="external-build/cpm65"
readonly URL="https://github.com/davidgiven/cpm65.git"
readonly BASEDIR="${PWD}"
readonly BUILDTARGET="sorbus.zip"
readonly TARGETDIR="${BASEDIR}/src/bin/cpm"

for i in $(readlink -f ../llvm-mos) /usr /usr/local /opt/llvm-mos; do
   if [ -x "${i}/bin/mos-cpm65-clang" ]; then
      LLVM_PATH="${i}"
      break
   fi
done

if [ -z "${LLVM_PATH}" ]; then
   echo "llvm-mos-sdk not found"
   exit 1
fi
readonly LLVM_PATH
echo "Using llvm-mos-sdk at ${LLVM_PATH}"

set -e

rm -rf "${WORKDIR}"
mkdir -p "${WORKDIR}"
cd "${WORKDIR}"
git clone --depth 1 "${URL}" "${PWD}"
make "${BUILDTARGET}" LLVM="${LLVM_PATH}/bin"
readonly ZIPDIR="${PWD}"
cd "${TARGETDIR}"
7z x -y "${ZIPDIR}/${BUILDTARGET}" CPMFS CPM BDOS
cpmcp -f sorbus CPMFS '0:*.*' 0/
rm -f CPMFS

echo "Success!"
