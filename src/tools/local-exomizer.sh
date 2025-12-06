#!/bin/sh

set -e
readonly BUILD_DIR="$(dirname "${0}")/../../../local"
readonly STOW_DIR="/usr/local/stow"
readonly PACKAGE="exomizer-3.1.2"
readonly URL="https://bitbucket.org/magli143/exomizer/wiki/downloads/${PACKAGE}.zip"

# check if exomizer is already installed
if [ -d "${STOW_DIR}/${PACKAGE}" -a "${1}" != "force" ]; then
   echo "${PACKAGE} already installed, skipping..."
   exit 0
fi

# setup build environment
rm -rf "${BUILD_DIR}/${PACKAGE}"
mkdir -p "${BUILD_DIR}/${PACKAGE}"
cd "${BUILD_DIR}"
[ -f "$(basename "${URL}")" ] || wget "${URL}"
cd "${BUILD_DIR}/${PACKAGE}"
7z x "../$(basename "${URL}")"

# build
jobs="$(nproc || echo 4)"
make -C src -j${jobs}

# wrapper printing what to be done with sudo
sudop()
{
   echo sudo "${@}"
   sudo "${@}"
}

if [ -w "${STOW_DIR}/${PACKAGE}" -o -w "${STOW_DIR}" -o -w "${STOW_DIR}/.." ]; then
   SUDO=""
else
   SUDO="sudop"
fi

# deploy manually (no "make install")
${SUDO} install -s -m 0755 -D src/exomizer "${STOW_DIR}/${PACKAGE}/bin/exomizer"
${SUDO} install -s -m 0755 -D src/exobasic "${STOW_DIR}/${PACKAGE}/bin/exobasic"

for file in *.txt $(find exodecrs rawdecrs); do
   ${SUDO} install "${file}" -m 0644 -D "${STOW_DIR}/${PACKAGE}/share/doc/${PACKAGE}/${file}"
done

# stow it
cd "${STOW_DIR}"
if [ -w "../bin" ]; then
   stow -Dv "${PACKAGE}-"*
   stow -v "${PACKAGE}-${version}"
else
   sudo stow -Dv "${PACKAGE}-"*
   sudo stow -v "${PACKAGE}-${version}"
fi
