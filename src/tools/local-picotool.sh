#!/bin/bash

# this one is a little ugly, because of
# - picotool repository needs to be fetched for version number
# - picotool requires pico-sdk to be downloaded as well
# - stuff typically static needs to be generated before including
#   common code

readonly URL="https://github.com/raspberrypi/picotool.git"
readonly PACKAGE_BASE="picotool"

set -eu
# save directory for later
pushd "$(dirname "${0}")/../.." >/dev/null
TOPDIR="${PWD}"

git_checkout="$(grep ^GIT_CHECKOUT Makefile | cut -f2 -d=)"
readonly PICOTOOL_DIR="$(dirname "${PWD}")/picotool"

# for getting the version, the source code needs to be already here
if [ ! -d "${PICOTOOL_DIR}" ]; then
   git clone "${URL}" "${PICOTOOL_DIR}"
   cd "${PICOTOOL_DIR}"
else
   cd "${PICOTOOL_DIR}"
   git pull
fi
readonly VERSION="$(grep 'set(PICOTOOL_VERSION' "CMakeLists.txt" | sed 's/set(PICOTOOL_VERSION  *\([^)]*\))/\1/')"
readonly PACKAGE="picotool-${VERSION}"

PICO_SDK_PATH="$(readlink -f ..)/pico-sdk"
export PICO_SDK_PATH

make -C "${TOPDIR}" "${PICO_SDK_PATH}/README.md"

# restore current directory to start
popd >/dev/null
. "$(dirname "${0}")/local-common.sh"

readonly build_dir="${BUILD_DIR}/picotool-build"

rm -rf "${build_dir}"
mkdir -p "${build_dir}"
cd "${build_dir}"

cmake "${PICOTOOL_DIR}"
make -j${JOBS}

sed -e "s,\(CMAKE_INSTALL_PREFIX:PATH\)=/usr/local,\1=${STOW_DIR}/${PACKAGE},g" \
    -i CMakeCache.txt

make PICO_SDK_PATH="${PICO_SDK_PATH}"
${sudo_package} make install PICO_SDK_PATH="${PICO_SDK_PATH}"

stow_package
