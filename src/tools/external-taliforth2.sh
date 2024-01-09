#!/bin/sh

cd "$(dirname "${0}")/../.."

readonly WORKDIR="external-build/TaliForth2"
readonly URL="https://github.com/SamCoVT/TaliForth2.git"
readonly BASEDIR="${PWD}"
readonly BUILDTARGET="taliforth-sorbus.bin"
readonly TARGETDIR="${BASEDIR}/src/bin/cpm/10"

set -e

rm -rf "${WORKDIR}"
mkdir -p "${WORKDIR}"
cd "${WORKDIR}"
git clone --depth 1 "${URL}" .
make "${BUILDTARGET}"
cp -v "${BUILDTARGET}" "${TARGETDIR}/tali4th2.sx4"

echo "Success!"
