#!/bin/sh

set -e

cd "$(dirname "${0}")/../.."

readonly basedir="${PWD}"
readonly repository_url="https://github.com/SamCoVT/TaliForth2.git"
readonly buildtarget="taliforth-sorbus.bin"
readonly distdir="${basedir}/src/bin/cpm/10"
git_checkout="$(grep ^GIT_CHECKOUT Makefile | cut -f2 -d=)"

cd ..

readonly target_dir="${PWD}/TaliForth2"

if [ -d "${target_dir}" ]; then
   cd "${target_dir}"
   git pull
   cd - 2>/dev/null
else
   ${git_checkout} "${repository_url}" "${target_dir}"
fi
cd "${target_dir}"

echo "check if the required assembler is available"
type 64tass

make "${buildtarget}"
cp -v "${buildtarget}" "${distdir}/tali4th2.sx4"

echo "Success!"

