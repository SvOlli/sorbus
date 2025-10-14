#!/bin/sh

set -e

cd "$(dirname "${0}")/../.."

readonly basedir="${PWD}"
readonly repository_url="https://git.h8u.de/Sorbus/1kLEDsIsNoLimit.git"
readonly buildtarget="nolimit.sx4"
readonly distdir="${basedir}/src/bin/cpm/9"
git_checkout="$(grep ^GIT_CHECKOUT Makefile | cut -f2 -d=)"

cd ..

readonly target_dir="${PWD}/1kLEDsIsNoLimit"

if [ -d "${target_dir}" ]; then
   cd "${target_dir}"
   git pull
   cd - 2>/dev/null
else
   ${git_checkout} "${repository_url}" "${target_dir}"
fi
cd "${target_dir}"

make clean "${buildtarget}"
cp -v "${buildtarget}" "${distdir}"

echo "Success!"

