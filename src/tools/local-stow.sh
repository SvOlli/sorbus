#!/bin/bash

set -e
readonly BUILD_DIR="$(dirname "${0}")/../../../local"
readonly STOW_DIR="/usr/local/stow"
readonly PACKAGE="nstow-1.0"
readonly URL="https://www.gnusto.com/src/${PACKAGE}.tar.gz"

# check if nstow is already installed
if [ -d "${STOW_DIR}/${PACKAGE}" -a "${1}" != "force" ]; then
   echo "${PACKAGE} already installed, skipping..."
   exit 0
fi

# setup build environment
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
[ -f "$(basename "${URL}")" ] || wget "${URL}"
tar xf "$(basename "${URL}")"

# run the build with small adjustments
rm -rf nstow-build
mkdir -p nstow-build
cd nstow-build
../${PACKAGE}/configure --prefix=/usr/local
sed -e 's,/\* #undef STDC_HEADERS \*/,#define STDC_HEADERS 1,g' \
    -i config.h   # starting with Debian Trixie, configure does not find stdio.h
make

if [ -w "${STOW_DIR}/${PACKAGE}" -o -w "${STOW_DIR}" -o -w "${STOW_DIR}/.." ]; then
   make install prefix="${STOW_DIR}/${PACKAGE}"
   mkdir "${STOW_DIR}/${PACKAGE}/share"
   mv "${STOW_DIR}/${PACKAGE}/man" "${STOW_DIR}/${PACKAGE}/share/man"
else
   sudo make install prefix="${STOW_DIR}/${PACKAGE}"
   sudo mkdir "${STOW_DIR}/${PACKAGE}/share"
   sudo mv "${STOW_DIR}/${PACKAGE}/man" "${STOW_DIR}/${PACKAGE}/share/man"
fi

# stow it with itself
cd "${STOW_DIR}"
if [ -w "../bin" ]; then
   ${PACKAGE}/bin/stow -v "${PACKAGE}"
else
   sudo ${PACKAGE}/bin/stow -v "${PACKAGE}"
fi
