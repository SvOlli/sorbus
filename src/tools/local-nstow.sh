#!/bin/bash

readonly PACKAGE="nstow-1.0"
readonly PACKAGE_BASE="${PACKAGE%-*}"
readonly URL="https://www.gnusto.com/src/${PACKAGE}.tar.gz"

. "$(dirname "${0}")/local-common.sh"

# setup build environment
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

${sudo_package} rm -rf "${STOW_DIR}/${PACKAGE}"/*
${sudo_package} make install prefix="${STOW_DIR}/${PACKAGE}"
${sudo_package} mkdir "${STOW_DIR}/${PACKAGE}/share"
${sudo_package} mv "${STOW_DIR}/${PACKAGE}/man" "${STOW_DIR}/${PACKAGE}/share/man"

cd "${STOW_DIR}"
${sudo_links} ${PACKAGE}/bin/stow -D "${PACKAGE_BASE}"-*
${sudo_links} ${PACKAGE}/bin/stow -v "${PACKAGE}"
