#!/bin/sh

set -e

# If something breaks during compiling from source (required when building on
# non-X86_64 architectures), you can uncomment these hashes and try again.
#llvm_mos_hash="2f5f0aca548bd4157633cf7d562e15875a8147f4"
#llvm_mos_sdk_hash="675461c3c095488b87c42d9a55df76210ed43e10"

# Force number of jobs in case RAM is not sufficient
#jobs=4

cd "$(dirname "${0}")/../.."

cd ..

readonly target_dir="${PWD}/llvm-mos-sdk"
readonly build_dir="${target_dir}-build"
[ -n "${jobs}" ] || jobs="$(nproc || echo 4)"

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

git_get()
{
   repo="${1}"
   dir="${2}"
   hash="${3}"

   if [ -d "${dir}" ]; then
      cd "${dir}"
      git pull
      if [ -n "${hash}" ]; then
         echo "using hash: ${hash}"
         git checkout "${hash}"
      fi
      cd - >/dev/null
   else
      git clone --depth=1 --recurse-submodules --shallow-submodules \
         "https://github.com/llvm-mos/${repo}.git" "${dir}"
      if [ -n "${hash}" ]; then
         cd "${dir}"
         echo "using hash: ${hash}"
         git checkout "${hash}"
         cd - >/dev/null
      fi
   fi
}

source()
{
   # verify that ninja is available
   type ninja

   mkdir -p "${build_dir}"
   cd "${build_dir}"

   echo "downloading source distribution from git"

   setversion

   local llvm_dir="llvm-mos"
   local sdk_dir="llvm-mos-sdk"

   git_get llvm-mos     "${llvm_dir}" "${llvm_mos_hash}"
   git_get llvm-mos-sdk "${sdk_dir}"  "${llvm_mos_sdk_hash}"

   cmake -C "${llvm_dir}/clang/cmake/caches/MOS.cmake" -G Ninja \
      -S "${llvm_dir}/llvm" -B "build-llvm" \
      -DCMAKE_INSTALL_PREFIX="${target_dir}" \
      -DCMAKE_BUILD_TYPE=Release
   cmake --build build-llvm --config Release --target all -- -j ${jobs}
   cmake --build build-llvm --config Release --target install

   # verify that mos-clang is executable
   type mos-clang

   cmake -S "${sdk_dir}" -B build-llvm-sdk -G Ninja \
      -DCMAKE_INSTALL_PREFIX="${target_dir}" \
      -DCMAKE_PREFIX_PATH="${target_dir}" \
      -DLLVM_MOS_BOOTSTRAP_COMPILER="OFF"
   cmake --build build-llvm-sdk --config Release --target all -- -j ${jobs}
   cmake --build build-llvm-sdk --config Release --target install

   echo
   echo "${build_dir} can now be deleted"
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

If building from source fails, take a look at top of this script for
additional hardcoded parameters.

EOM
exit 0
esac

echo
echo "Success"
echo

exit 0

