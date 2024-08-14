#!/bin/sh

set -e

cd "$(dirname "${0}")/../.."

cd ..

readonly target_dir="${PWD}/llvm-mos-sdk"
readonly build_dir="${target_dir}-build"
jobs="$(nproc || echo 4)"

readonly machinetype="$(uname -m)"
readonly systype="$(uname -s)"
case "${systype}" in
Linux) ;;
*) cat <<EOM
Unknown system tyype '${systype}'.
If you want something else besides 'Linux', please send a patch.
EOM
   exit 2;;
esac

# make sure that the new toolchain will be found by the SDK
PATH="${target_dir}/bin:${PATH}"
export PATH

setversion()
{
   version="$(wget -O - https://github.com/llvm-mos/llvm-mos-sdk/releases |
              grep SDK\ v | head -1 | sed -e 's/.*>SDK \(v[^<]*\)<.*/\1/')"

   echo "current version is ${version}"
}

download()
{
   local url="${1}"
   shift
   local basename="$(basename "${url}")"

   if [ ! -f "${basename}" ]; then
      echo "${*}"
      wget "${url}"
   fi
}

source()
{
   # verify that ninja is available
   type ninja

   mkdir -p "${build_dir}"
   cd "${build_dir}"

   echo "downloading source distribution"

   setversion

   local llvm_archive="${PWD}/llvm-mos-linux-debug.tar.gz"
   local sdk_archive="${PWD}/${version}.tar.gz"
   local llvm_dir="llvm-mos-llvm-mos-linux-debug"
   local sdk_dir="llvm-mos-sdk-$(echo ${version} | cut -c2-)"

   download \
      https://github.com/llvm-mos/llvm-mos/archive/refs/tags/llvm-mos-linux-debug.tar.gz \
      "downloading llvm-mos source code (~200MB)"
   download \
      https://github.com/llvm-mos/llvm-mos-sdk/archive/refs/tags/${version}.tar.gz \
      "downloading llvm-mos-sdk source code (~0.5MB)"

   if [ ! -d "${llvm_dir}" ]; then
      echo "unpacking llvm"
      tar xf "${llvm_archive}"
   fi
   cmake -C "${llvm_dir}/clang/cmake/caches/MOS.cmake" -G Ninja \
      -S "${llvm_dir}/llvm" -B "build-llvm" \
      -DCMAKE_INSTALL_PREFIX="${target_dir}" \
      -DCMAKE_BUILD_TYPE=Release
   cmake --build build-llvm --config Release --target all -- -j ${jobs}
   cmake --build build-llvm --config Release --target install

   # verify that mos-clang is executable
   type mos-clang

   if [ ! -d "${sdk_dir}" ]; then
      echo "unpacking sdk"
      tar xf "${sdk_archive}"
   fi

   # this break compilation with v19.1.0
   sed -e 's/typedef unsigned size_t;/typedef __SIZE_TYPE__ size_t;/' \
       -i "${sdk_dir}/mos-platform/common/include/unistd.h"

   cmake -S "${sdk_dir}" -B build-llvm-sdk -G Ninja \
      -DCMAKE_INSTALL_PREFIX="${target_dir}" \
      -DCMAKE_PREFIX_PATH="${target_dir}" \
      -DLLVM_MOS_BOOTSTRAP_COMPILER="OFF"
   cmake --build build-llvm-sdk --config Release --target all -- -j ${jobs}
   cmake --build build-llvm-sdk --config Release --target install
}

binary_linux()
{
   echo "downloading binary distribution for x86_64"
   setversion

   mkdir -p "${build_dir}"
   cd "${build_dir}"

   download \
      https://github.com/llvm-mos/llvm-mos-sdk/releases/download/${version}/llvm-mos-linux.tar.xz \
      "downloading llvm-mos binary distribution (~100MB)"

   local archive="$(readlink -f llvm-mos-linux.tar.xz)"

   mkdir -p "${target_dir}"
   cd "${target_dir}"
   tar --strip-components=1 -xvf "${archive}"
}

case "${1}" in
clean) rm -rf "${build_dir}"; shift;;
esac

case "${1}_${machinetype}" in
binary_x86_64|_x86_64) binary_linux;;
source_*|_*) source;;
*) cat <<EOM
Usage: $0 (clean) (source|binary)

No argument uses binary distribution on x86_64, source in any other case.

EOM
exit 0
esac

echo
echo "Success"
echo

exit 0

