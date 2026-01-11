#!/bin/sh

set -e
cd "$(dirname "${0}")/../.."

readonly BUILD_DIR="$(dirname "${PWD}")/../local"
readonly STOW_DIR="/usr/local/stow"
readonly PACKAGE="picotool"
readonly URL="https://github.com/raspberrypi/picotool.git"

git_checkout="$(grep ^GIT_CHECKOUT Makefile | cut -f2 -d=)"

PICO_SDK_PATH="$(readlink -f ..)/pico-sdk"
export PICO_SDK_PATH

make "${PICO_SDK_PATH}/README.md"

cd ..

readonly target_dir="${PWD}/picotool"
readonly build_dir="${BUILD_DIR}/picotool-build"
jobs="$(nproc || echo 4)"

rm -rf "${build_dir}"
mkdir -p "${build_dir}"
cd "${build_dir}"
pwd

if [ -d "${target_dir}" ]; then
   cd "${target_dir}"
   git pull
   cd - 2>/dev/null
else
   ${git_checkout} "${URL}" "${target_dir}"
fi

cmake "${target_dir}"
make -j${jobs}

version="$(./picotool version | cut -f1-2 -d\  | head -1 | tr ' ' -)"
sed -e "s,\(CMAKE_INSTALL_PREFIX:PATH\)=/usr/local,\1=${STOW_DIR}/${version},g" \
    -i CMakeCache.txt

if [ -w "${STOW_DIR}/${version}" -o -w "${STOW_DIR}" -o -w "${STOW_DIR}/.." ]; then
   make install PICO_SDK_PATH="${PICO_SDK_PATH}"
else
   sudo make install PICO_SDK_PATH="${PICO_SDK_PATH}"
fi

# stow it
cd "${STOW_DIR}"
if [ -w "../bin" ]; then
   stow -Dv "${PACKAGE}-"*
   stow -v "${version}"
else
   sudo stow -Dv "${PACKAGE}-"*
   sudo stow -v "${version}"
fi
