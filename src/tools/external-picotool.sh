#!/bin/sh

set -e

cd "$(dirname "${0}")/../.."

git_checkout="$(grep ^GIT_CHECKOUT Makefile | cut -f2 -d=)"

PICO_SDK_PATH="$(readlink -f ..)/pico-sdk"
export PICO_SDK_PATH

make "${PICO_SDK_PATH}/README.md"

cd ..

readonly target_dir="${PWD}/picotool"
readonly build_dir="${target_dir}-build"
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
   ${git_checkout} https://github.com/raspberrypi/picotool.git "${target_dir}"
fi

cmake "${target_dir}"
make

mkdir -p "${HOME}/bin"
cp "picotool" "${HOME}/bin"
ls -l "${HOME}/bin/picotool"

