#!/bin/sh

set -e

cd "$(dirname "${0}")/../.."

readonly basedir="${PWD}"
git_checkout="$(grep ^GIT_CHECKOUT Makefile | cut -f2 -d=)"

cd ..

readonly target_dir="${PWD}/cpm65"
readonly repository_url="https://github.com/davidgiven/cpm65.git"
readonly buildtarget="sorbus.zip"
readonly distdir="${basedir}/src/bin/cpm"

for i in ${PWD}/llvm-mos-sdk /usr /usr/local /opt/llvm-mos; do
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

if [ -d "${target_dir}" ]; then
   cd "${target_dir}"
   git pull
   cd - 2>/dev/null
else
   ${git_checkout} "${repository_url}" "${target_dir}"
fi
cd "${target_dir}"

make "${buildtarget}" LLVM="${LLVM_PATH}/bin"
cd "${distdir}"
7z x -y "${target_dir}/${buildtarget}" CPMFS CPM BDOS
cpmcp -f sorbus CPMFS '0:*.*' 0/
rm -f CPMFS

echo "Success!"

